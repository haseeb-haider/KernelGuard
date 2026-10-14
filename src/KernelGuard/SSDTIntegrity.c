#pragma warning(disable: 4100)
#include "SSDTIntegrity.h"
#include "Utilities.h"

// Phase 2 TODO: resolve KeServiceDescriptorTable (or the appropriate
// per-OS-build equivalent on modern x64 Windows, since the symbol isn't
// exported past certain builds), copy out ServiceTableBase / NumberOfServices,
// and store a baseline array of entry addresses here.
VOID KgSsdtIntegrityBaseline(VOID)
{
	KgLog(KG_LOG_INFO, "SSDT integrity baseline: not yet implemented (Phase 2)\n");
}

// Phase 2 TODO: re-read the same table, compare each entry against the
// baseline captured above, and KgLog(KG_LOG_ALERT, ...) on any entry whose
// address has moved outside the expected kernel image range.
VOID KgSsdtIntegrityCheck(VOID)
{
	KgLog(KG_LOG_INFO, "SSDT integrity check: stub, no baseline captured yet\n");
}
