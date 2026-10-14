#pragma warning(disable: 4100)
#pragma warning(disable: 4005)
#pragma warning(disable: 4668)
#pragma warning(disable: 4820)
#include "Monitor.h"
#include "Utilities.h"
#include "SSDTIntegrity.h"
#include "IDTIntegrity.h"
#include "ProcessIntegrity.h"

// How often the worker re-runs every *Check() function, in milliseconds.
// 5s is a reasonable default for Phase 1 scaffolding/demo purposes; tune
// down once real diffing logic lands and the per-tick cost is known.
#define KG_MONITOR_INTERVAL_MS 5000

static PETHREAD g_MonitorThread = NULL;
static KEVENT g_StopEvent;
static volatile BOOLEAN g_StopRequested = FALSE;

static VOID KgMonitorWorker(PVOID Context)
{
	LARGE_INTEGER interval;

	UNREFERENCED_PARAMETER(Context);

	// Negative values to KeWaitForSingleObject's timeout mean "relative to
	// now", in 100ns units — hence the *10000 to go from milliseconds.
	interval.QuadPart = -((LONGLONG)KG_MONITOR_INTERVAL_MS * 10000);

	KgLog(KG_LOG_INFO, "Monitor worker started (interval=%lu ms)\n", KG_MONITOR_INTERVAL_MS);

	while (!g_StopRequested)
	{
		NTSTATUS waitStatus = KeWaitForSingleObject(&g_StopEvent, Executive, KernelMode, FALSE, &interval);

		if (waitStatus == STATUS_SUCCESS)
		{
			// g_StopEvent was signaled — KgMonitorStop() wants us to exit.
			break;
		}

		// STATUS_TIMEOUT: interval elapsed with no stop request — run a tick.
		KgSsdtIntegrityCheck();
		KgIdtIntegrityCheck();
		KgProcessIntegrityCheck();
	}

	KgLog(KG_LOG_INFO, "Monitor worker exiting\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS KgMonitorStart(VOID)
{
	NTSTATUS status;
	HANDLE threadHandle;
	OBJECT_ATTRIBUTES objAttr;

	KeInitializeEvent(&g_StopEvent, NotificationEvent, FALSE);
	g_StopRequested = FALSE;

	KgSsdtIntegrityBaseline();
	KgIdtIntegrityBaseline();
	KgProcessIntegrityBaseline();

	InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &objAttr, NULL, NULL, KgMonitorWorker, NULL);
	if (!NT_SUCCESS(status))
	{
		KgLog(KG_LOG_ALERT, "Failed to start monitor thread: 0x%08X\n", status);
		return status;
	}

	// Keep a referenced PETHREAD (not just the HANDLE) so KgMonitorStop()
	// can KeWaitForSingleObject() on the thread object after the handle
	// itself has been closed.
	status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&g_MonitorThread, NULL);
	ZwClose(threadHandle);

	if (!NT_SUCCESS(status))
	{
		KgLog(KG_LOG_ALERT, "Failed to reference monitor thread object: 0x%08X\n", status);
		g_MonitorThread = NULL;
		return status;
	}

	return STATUS_SUCCESS;
}

VOID KgMonitorStop(VOID)
{
	if (g_MonitorThread == NULL)
	{
		return;
	}

	g_StopRequested = TRUE;
	KeSetEvent(&g_StopEvent, IO_NO_INCREMENT, FALSE);

	KeWaitForSingleObject(g_MonitorThread, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(g_MonitorThread);
	g_MonitorThread = NULL;
}
