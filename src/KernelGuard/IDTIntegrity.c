#pragma warning(disable: 4100)
#include "IDTIntegrity.h"
#include "Utilities.h"

// Phase 2 TODO: use __sidt / KeGetPcr()-relative IDT base on x64 to read
// each gate descriptor (offset + selector) per logical processor and store
// a baseline. Also snapshot __readmsr(IA32_SYSENTER_EIP) alongside the IDT
// baseline, since SYSENTERHook.c modifies that MSR rather than the IDT
// itself.
VOID KgIdtIntegrityBaseline(VOID)
{
	KgLog(KG_LOG_INFO, "IDT/MSR integrity baseline: not yet implemented (Phase 2)\n");
}

// Phase 2 TODO: re-read the IDT and IA32_SYSENTER_EIP, diff against the
// baseline per-core (hooks can be installed on one core only), and
// KgLog(KG_LOG_ALERT, ...) on mismatch.
VOID KgIdtIntegrityCheck(VOID)
{
	KgLog(KG_LOG_INFO, "IDT/MSR integrity check: stub, no baseline captured yet\n");
}
