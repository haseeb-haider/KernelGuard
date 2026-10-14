#pragma warning(disable: 4100)
#pragma warning(disable: 4005)
#pragma warning(disable: 4668)
#pragma warning(disable: 4820)
#include "Driver.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(pRegistryPath);

	pDriverObject->DriverUnload = DriverUnload;

	KgLog(KG_LOG_INFO, "KernelGuard loading\n");

	status = KgMonitorStart();
	if (!NT_SUCCESS(status))
	{
		KgLog(KG_LOG_ALERT, "Failed to start integrity monitor: 0x%08X\n", status);
		return status;
	}

	KgLog(KG_LOG_INFO, "KernelGuard loaded, monitor running\n");
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);

	KgLog(KG_LOG_INFO, "KernelGuard unloading\n");
	KgMonitorStop();
	KgLog(KG_LOG_INFO, "KernelGuard unloaded\n");
}
