#pragma once
#include <ntddk.h>

// Phase 1: scaffolding only. KgSsdtIntegrityBaseline() is called once at
// monitor startup and is meant to snapshot every KeServiceDescriptorTable
// entry address; KgSsdtIntegrityCheck() is called on each monitor tick and
// is meant to re-walk the table and diff against that snapshot, raising
// KG_LOG_ALERT on any mismatch (the signature of SSDTHook.c in
// Kernelious-Rootkit). Both are no-ops that just log until Phase 2.
VOID KgSsdtIntegrityBaseline(VOID);
VOID KgSsdtIntegrityCheck(VOID);
