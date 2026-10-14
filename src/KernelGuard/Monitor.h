#pragma once
#include <ntddk.h>

// Starts the periodic integrity-check worker thread. Calls each check
// module's *Baseline() once, then spawns a system thread that calls each
// module's *Check() on a fixed interval until KgMonitorStop() is called.
// Returns an error if the worker thread could not be created.
NTSTATUS KgMonitorStart(VOID);

// Signals the worker thread to exit and blocks until it has terminated.
// Safe to call even if KgMonitorStart() was never called or already failed.
VOID KgMonitorStop(VOID);
