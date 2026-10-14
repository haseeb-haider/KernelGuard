#pragma once
#pragma warning(push)
#pragma warning(disable: 4005)  // Suppress macro redefinition warnings
#pragma warning(disable: 4668)  // Suppress undefined preprocessor macro warnings
#pragma warning(disable: 4820)  // Suppress padding warnings
#include <ntddk.h>
#include <wdm.h>
#include <stdarg.h>
#pragma warning(pop)

// Logging levels understood by KgLog(). Kept as plain ints (not an enum) so
// callers can pass DPFLTR_* constants directly if they need finer control
// later without changing this header.
#define KG_LOG_INFO  0
#define KG_LOG_ALERT 1

// Thin wrapper around vDbgPrintExWithPrefix so every KernelGuard component
// logs through one choke point with a consistent "[KernelGuard]" prefix.
// KG_LOG_ALERT additionally tags the line as "ALERT:" and logs at
// DPFLTR_ERROR_LEVEL so tamper detections stand out from routine status
// messages in the debugger output / kernel log.
void KgLog(ULONG Level, PCSTR Format, ...);
