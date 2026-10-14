#pragma warning(disable: 4100)
#pragma warning(disable: 4005)
#pragma warning(disable: 4668)
#pragma warning(disable: 4820)
#include "Utilities.h"

void KgLog(ULONG Level, PCSTR Format, ...)
{
	va_list args;
	PCSTR prefix;
	ULONG dbgLevel;

	if (Level == KG_LOG_ALERT)
	{
		prefix = "[KernelGuard] ALERT: ";
		dbgLevel = DPFLTR_ERROR_LEVEL;
	}
	else
	{
		prefix = "[KernelGuard] ";
		dbgLevel = DPFLTR_INFO_LEVEL;
	}

	va_start(args, Format);
	vDbgPrintExWithPrefix(prefix, DPFLTR_IHVDRIVER_ID, dbgLevel, Format, args);
	va_end(args);
}
