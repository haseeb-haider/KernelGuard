#pragma warning(disable: 4100)
#include "IDTIntegrity.h"
#include "Utilities.h"
#include <intrin.h>

#define KG_IDT_MAX_ENTRIES 256
#define IA32_LSTAR 0xC0000082ULL

#pragma pack(push, 1)
typedef struct _KG_IDTR
{
    UINT16 Limit;
    UINT64 Base;
} KG_IDTR;

typedef struct _KG_IDT_GATE
{
    UINT16 OffsetLow;
    UINT16 Selector;
    UINT8  IstAndReserved;
    UINT8  TypeAttr;
    UINT16 OffsetMid;
    UINT32 OffsetHigh;
    UINT32 Reserved;
} KG_IDT_GATE;
#pragma pack(pop)

static PVOID g_IdtBaseline[KG_IDT_MAX_ENTRIES];
static ULONG g_IdtEntryCount = 0;
static ULONGLONG g_LstarBaseline = 0;

static PVOID KgGateIsrAddress(const KG_IDT_GATE* Gate)
{
    ULONG64 low = Gate->OffsetLow;
    ULONG64 mid = Gate->OffsetMid;
    ULONG64 high = Gate->OffsetHigh;
    return (PVOID)(low | (mid << 16) | (high << 32));
}

// Reads the current IDT into OutIsrArray (capped at MaxEntries) and returns
// how many entries were captured. Must be called with thread affinity
// pinned to a single CPU — each logical processor has its own IDTR.
static ULONG KgReadIdt(PVOID* OutIsrArray, ULONG MaxEntries)
{
    KG_IDTR idtr;
    KG_IDT_GATE* gates;
    ULONG entryCount;

    __sidt(&idtr);

    entryCount = (ULONG)((idtr.Limit + 1) / sizeof(KG_IDT_GATE));
    if (entryCount > MaxEntries)
    {
        entryCount = MaxEntries;
    }

    gates = (KG_IDT_GATE*)idtr.Base;
    for (ULONG i = 0; i < entryCount; i++)
    {
        OutIsrArray[i] = KgGateIsrAddress(&gates[i]);
    }

    return entryCount;
}

// Phase 2 limitation: only CPU 0's IDT is captured/compared. A hook planted
// only on another logical processor's IDT won't be caught until a future
// phase pins per-core baselines (KeSetSystemAffinityThread with each core's
// mask in turn).
VOID KgIdtIntegrityBaseline(VOID)
{
    KeSetSystemAffinityThread((KAFFINITY)1);

    g_IdtEntryCount = KgReadIdt(g_IdtBaseline, KG_IDT_MAX_ENTRIES);
    g_LstarBaseline = __readmsr(IA32_LSTAR);

    KeRevertToUserAffinityThread();

    KgLog(KG_LOG_INFO, "IDT/MSR integrity baseline captured: %lu IDT entries, LSTAR=%p\n",
          g_IdtEntryCount, (PVOID)g_LstarBaseline);
}

VOID KgIdtIntegrityCheck(VOID)
{
    PVOID current[KG_IDT_MAX_ENTRIES];
    ULONG count;
    ULONG checkCount;
    ULONGLONG currentLstar;

    if (g_IdtEntryCount == 0)
    {
        KgLog(KG_LOG_INFO, "IDT/MSR integrity check: no baseline captured, skipping\n");
        return;
    }

    KeSetSystemAffinityThread((KAFFINITY)1);
    count = KgReadIdt(current, KG_IDT_MAX_ENTRIES);
    currentLstar = __readmsr(IA32_LSTAR);
    KeRevertToUserAffinityThread();

    if (currentLstar != g_LstarBaseline)
    {
        KgLog(KG_LOG_ALERT, "LSTAR (SYSCALL entry MSR) changed: baseline=%p current=%p\n",
              (PVOID)g_LstarBaseline, (PVOID)currentLstar);
    }

    checkCount = (count < g_IdtEntryCount) ? count : g_IdtEntryCount;
    for (ULONG i = 0; i < checkCount; i++)
    {
        if (current[i] != g_IdtBaseline[i])
        {
            KgLog(KG_LOG_ALERT, "IDT entry %lu changed: baseline=%p current=%p\n",
                  i, g_IdtBaseline[i], current[i]);
        }
    }
}
