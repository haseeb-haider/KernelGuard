#pragma once
#include <ntddk.h>

// Phase 1: scaffolding only. Mirrors SSDTIntegrity.h but for IDT gate
// descriptors (the target of IDTHook.c in Kernelious-Rootkit). Phase 2
// should snapshot each gate's offset/selector via SIDT + per-core reads,
// and Phase 3+ should also cover the IA32_SYSENTER_EIP MSR path targeted
// by SYSENTERHook.c, since both land in the same "control-transfer
// integrity" bucket.
VOID KgIdtIntegrityBaseline(VOID);
VOID KgIdtIntegrityCheck(VOID);
