#pragma once
#include <ntddk.h>

// Locates nt!KeServiceDescriptorTable, snapshots every service-table entry
// address, and caches the table location so KgSsdtIntegrityCheck() doesn't
// have to re-scan ntoskrnl on every tick. Called once at monitor startup.
// Logs KG_LOG_ALERT (instead of crashing) if the table can't be located on
// this Windows build.
VOID KgSsdtIntegrityBaseline(VOID);

// Re-reads the cached table and raises KG_LOG_ALERT on any entry whose
// resolved address no longer matches the baseline (the signature of
// SSDTHook.c in Kernelious-Rootkit). No-op if the baseline capture failed.
VOID KgSsdtIntegrityCheck(VOID);
