#pragma warning(disable: 4100)
#include "ProcessIntegrity.h"
#include "Utilities.h"

// Phase 3 TODO: there is no meaningful "baseline" for the process list
// (it legitimately changes every tick) — this hook exists so the monitor
// framework is symmetric across checks. Phase 3 will likely use this to
// initialize whatever secondary enumeration structures the cross-check
// needs (e.g. a cached PID set from the previous tick).
VOID KgProcessIntegrityBaseline(VOID)
{
	KgLog(KG_LOG_INFO, "Process-list integrity baseline: not yet implemented (Phase 3)\n");
}

// Phase 3 TODO: walk PsGetNextProcess() into one PID set, walk
// PsInitialSystemProcess's ActiveProcessLinks directly into a second PID
// set, and KgLog(KG_LOG_ALERT, ...) for any PID present in the raw walk
// but absent from the official enumeration (the signature of EPROCESS
// unlinking).
VOID KgProcessIntegrityCheck(VOID)
{
	KgLog(KG_LOG_INFO, "Process-list integrity check: stub, cross-check not implemented yet\n");
}
