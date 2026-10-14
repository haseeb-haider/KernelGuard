#pragma once
#include <ntddk.h>

// Snapshots every IDT gate's ISR address plus IA32_LSTAR (the x64 SYSCALL
// entry-point MSR targeted by SYSENTERHook.c in Kernelious-Rootkit) on CPU 0.
// Called once at monitor startup.
VOID KgIdtIntegrityBaseline(VOID);

// Re-reads the same IDT/MSR state on CPU 0 and raises KG_LOG_ALERT on any
// entry that no longer matches the baseline. Called on each monitor tick.
VOID KgIdtIntegrityCheck(VOID);
