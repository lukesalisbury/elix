#ifndef ELIX_OS_WIN_HPP
#define ELIX_OS_WIN_HPP

#include "elix_core.h"

#define PSAPI_VERSION 2
#include <windows.h>
#include <psapi.h>


size_t elix_os_memory_usage() {
	PROCESS_MEMORY_COUNTERS pmc;
	if ( K32GetProcessMemoryInfo ( GetCurrentProcess(), &pmc, sizeof(pmc)) )
	{
		return pmc.WorkingSetSize;
//		LOG_MESSAGE( "\tPageFaultCount: 0x%08X", pmc.PageFaultCount );
//		LOG_MESSAGE( "\tPeakWorkingSetSize:%d", pmc.PeakWorkingSetSize );
//		LOG_MESSAGE( "\tWorkingSetSize:%d", pmc.WorkingSetSize );
//		LOG_MESSAGE( "\tQuotaPeakPagedPoolUsage:%zd", pmc.QuotaPeakPagedPoolUsage );
//		LOG_MESSAGE( "\tQuotaPagedPoolUsage:%zd", pmc.QuotaPagedPoolUsage );
//		LOG_MESSAGE( "\tQuotaPeakNonPagedPoolUsage:%zd", pmc.QuotaPeakNonPagedPoolUsage );
//		LOG_MESSAGE( "\tQuotaNonPagedPoolUsage:%zd", pmc.QuotaNonPagedPoolUsage );
//		LOG_MESSAGE( "\tPagefileUsage:%zd", pmc.PagefileUsage );
//		LOG_MESSAGE( "\tPeakPagefileUsage:%zd", pmc.PeakPagefileUsage );
	}
	return 0;
}

#endif // ELIX_SYSINFO_HPP
