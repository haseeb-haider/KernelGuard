#pragma once
#pragma warning(push)
#pragma warning(disable: 4005)  // Suppress macro redefinition warnings
#pragma warning(disable: 4668)  // Suppress undefined preprocessor macro warnings
#pragma warning(disable: 4820)  // Suppress padding warnings
#include <ntddk.h>
#include <wdm.h>
#pragma warning(pop)

#include "Utilities.h"
#include "Monitor.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
VOID DriverUnload(PDRIVER_OBJECT pDriverObject);
