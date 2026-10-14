#pragma warning(disable: 4100)
#include "SSDTIntegrity.h"
#include "Utilities.h"

#define KG_SSDT_MAX_ENTRIES 512

// Upper bound on how far past the ntoskrnl base we'll scan for the pattern
// below before giving up. Generous relative to where KiSystemServiceStart
// actually sits in every build we've checked, but still small enough that a
// failed scan terminates in well under a second instead of running off into
// unmapped memory.
#define KG_SSDT_SCAN_MAX_BYTES 0x400000

// Byte pattern identifying the `mov [rbx+90h], r12; mov edi, eax; shr edi, 7`
// sequence inside KiSystemServiceUser/KiSystemCall64 on supported x64
// builds, immediately followed (at KG_SSDT_PATTERN_OFFSET) by the
// `lea r10, KeServiceDescriptorTable` instruction. Same pattern used by
// Kernelious-Rootkit's SSDTHook.c, since KernelGuard has to find the exact
// table that project hooks.
static const UCHAR kSsdtPattern[] = { 0x48, 0x89, 0xa3, 0x90, 0x00, 0x00, 0x00, 0x8b, 0xf8, 0xc1, 0xef, 0x07 };
#define KG_SSDT_PATTERN_OFFSET 0x14

// Layout of nt!KeServiceDescriptorTable. NumberOfServices is really a
// 32-bit ULONG (Limit) padded out to 8 bytes to keep ParamTableBase
// pointer-aligned — modeled here as a ULONG64 so the struct layout lines up
// without needing an explicit padding field.
typedef struct _KG_SERVICE_DESCRIPTOR_TABLE
{
    PLONG  ServiceTableBase;
    PVOID  ServiceCounterTableBase;
    ULONG64 NumberOfServices;
    PUCHAR ParamTableBase;
} KG_SERVICE_DESCRIPTOR_TABLE, *PKG_SERVICE_DESCRIPTOR_TABLE;

typedef NTSYSAPI PVOID (*PFN_RTL_PC_TO_FILE_HEADER)(PVOID PcValue, PVOID* BaseOfImage);

static PLONG g_SsdtBase = NULL;
static ULONG g_SsdtCount = 0;
static PVOID g_SsdtBaseline[KG_SSDT_MAX_ENTRIES];

static PVOID KgGetNtoskrnlBase(VOID)
{
    UNICODE_STRING name;
    PFN_RTL_PC_TO_FILE_HEADER rtlPcToFileHeader;
    PVOID base = NULL;

    RtlInitUnicodeString(&name, L"RtlPcToFileHeader");
    rtlPcToFileHeader = (PFN_RTL_PC_TO_FILE_HEADER)MmGetSystemRoutineAddress(&name);
    if (rtlPcToFileHeader == NULL)
    {
        return NULL;
    }

    // Any exported ntoskrnl routine works as the probe address — resolving
    // the module that contains ZwQueryKey gives us ntoskrnl's base.
    rtlPcToFileHeader((PVOID)&ZwQueryKey, &base);
    return base;
}

static BOOLEAN KgMatchesPatternAt(PUCHAR Address)
{
    for (SIZE_T i = 0; i < sizeof(kSsdtPattern); i++)
    {
        if (!MmIsAddressValid(Address + i))
        {
            return FALSE;
        }
        if (Address[i] != kSsdtPattern[i])
        {
            return FALSE;
        }
    }
    return TRUE;
}

static PUCHAR KgFindSsdtPatternAddress(PUCHAR ScanStart)
{
    for (SIZE_T offset = 0; offset < KG_SSDT_SCAN_MAX_BYTES; offset++)
    {
        if (KgMatchesPatternAt(ScanStart + offset))
        {
            return ScanStart + offset;
        }
    }
    return NULL;
}

// Resolves nt!KeServiceDescriptorTable's address. Returns NULL (and logs)
// instead of crashing if the pattern isn't found or the table doesn't look
// like mapped memory — both are expected outcomes on an unsupported build,
// not something this driver should bugcheck over.
static PKG_SERVICE_DESCRIPTOR_TABLE KgLocateServiceDescriptorTable(VOID)
{
    PVOID ntoskrnlBase;
    PUCHAR patternAddress;
    PUCHAR leaInstruction;
    LONG relativeOffset;
    PUCHAR nextInstruction;
    PKG_SERVICE_DESCRIPTOR_TABLE sdt = NULL;

    ntoskrnlBase = KgGetNtoskrnlBase();
    if (ntoskrnlBase == NULL)
    {
        KgLog(KG_LOG_ALERT, "SSDT locate: could not resolve ntoskrnl base address\n");
        return NULL;
    }

    patternAddress = KgFindSsdtPatternAddress((PUCHAR)ntoskrnlBase);
    if (patternAddress == NULL)
    {
        KgLog(KG_LOG_ALERT, "SSDT locate: pattern not found within %d MB of ntoskrnl base - unsupported build\n",
              KG_SSDT_SCAN_MAX_BYTES / (1024 * 1024));
        return NULL;
    }

    __try
    {
        leaInstruction = patternAddress + KG_SSDT_PATTERN_OFFSET;
        relativeOffset = *(PLONG)(leaInstruction + 3);
        nextInstruction = leaInstruction + 7;
        sdt = (PKG_SERVICE_DESCRIPTOR_TABLE)(nextInstruction + relativeOffset);

        if (!MmIsAddressValid(sdt) || !MmIsAddressValid((PUCHAR)sdt + sizeof(KG_SERVICE_DESCRIPTOR_TABLE) - 1))
        {
            sdt = NULL;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KgLog(KG_LOG_ALERT, "SSDT locate: exception 0x%08X while resolving table address\n", GetExceptionCode());
        sdt = NULL;
    }

    return sdt;
}

VOID KgSsdtIntegrityBaseline(VOID)
{
    PKG_SERVICE_DESCRIPTOR_TABLE sdt = KgLocateServiceDescriptorTable();
    if (sdt == NULL)
    {
        g_SsdtBase = NULL;
        g_SsdtCount = 0;
        return;
    }

    __try
    {
        ULONG count = (ULONG)sdt->NumberOfServices;
        if (count > KG_SSDT_MAX_ENTRIES)
        {
            count = KG_SSDT_MAX_ENTRIES;
        }

        g_SsdtBase = sdt->ServiceTableBase;
        g_SsdtCount = count;

        for (ULONG i = 0; i < g_SsdtCount; i++)
        {
            LONG entry = g_SsdtBase[i];
            g_SsdtBaseline[i] = (PUCHAR)g_SsdtBase + (entry >> 4);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KgLog(KG_LOG_ALERT, "SSDT baseline: exception 0x%08X while reading table\n", GetExceptionCode());
        g_SsdtBase = NULL;
        g_SsdtCount = 0;
        return;
    }

    KgLog(KG_LOG_INFO, "SSDT integrity baseline captured: %lu entries, table at %p\n", g_SsdtCount, g_SsdtBase);
}

VOID KgSsdtIntegrityCheck(VOID)
{
    if (g_SsdtBase == NULL || g_SsdtCount == 0)
    {
        KgLog(KG_LOG_INFO, "SSDT integrity check: no baseline captured, skipping\n");
        return;
    }

    __try
    {
        for (ULONG i = 0; i < g_SsdtCount; i++)
        {
            LONG entry = g_SsdtBase[i];
            PVOID current = (PUCHAR)g_SsdtBase + (entry >> 4);

            if (current != g_SsdtBaseline[i])
            {
                KgLog(KG_LOG_ALERT, "SSDT entry %lu changed: baseline=%p current=%p\n",
                      i, g_SsdtBaseline[i], current);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KgLog(KG_LOG_ALERT, "SSDT check: exception 0x%08X while reading table\n", GetExceptionCode());
    }
}
