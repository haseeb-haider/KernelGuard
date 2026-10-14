#pragma once
#include <ntddk.h>

// Phase 1: scaffolding only. Targets EPROCESS active-process-link
// unlinking (the technique in Process.c in Kernelious-Rootkit). Phase 3
// should walk the process list two independent ways — e.g.
// PsGetNextProcess() vs. a raw ActiveProcessLinks traversal from
// PsInitialSystemProcess, or a cross-check against handle-table /
// thread-list enumeration — and alert on any PID present in one walk but
// missing from the other.
VOID KgProcessIntegrityBaseline(VOID);
VOID KgProcessIntegrityCheck(VOID);
