#ifdef PLATFORM_WINDOWS

#ifndef ELIX_OS_WIN_HPP
#define ELIX_OS_WIN_HPP

#include "elix_os.hpp"

#define PSAPI_VERSION 2
#include <windows.h>
#include <psapi.h>

#ifndef _WIN32_IE
	#define _WIN32_IE 0x0600
#endif
#include <io.h>
#include <shlobj.h>
#include <direct.h>

char * elix_program_directory__special_get(ELIX_PROGRAM_DIRECTORY dir_type, const char * folder_name, UNUSEDARG size_t extra_bytes) {
	static int32_t specials[ELIX_PROGRAM_DIRECTORY_LENGTH] = {
		CSIDL_COMMON_DOCUMENTS,
		CSIDL_PERSONAL,
		CSIDL_APPDATA,
		CSIDL_LOCAL_APPDATA
	};
	ASSERT(dir_type < ELIX_PROGRAM_DIRECTORY_LENGTH);

	char * directory = new char[ELIX_FILE_PATH_LENGTH]();
	if ( SHGetSpecialFolderPath(NULL, directory, specials[dir_type], 1) ) {
		elix_cstring_char_replace(directory, '\\', '/');
	} else {
		elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "c:/temp/", 8);
	}
	return directory;
}


void elix_os_system_idle(uint32_t time) {
	Sleep(time);
}

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

#include "elix_cstring.hpp"
#include "elix_file.hpp"
#include <Shlobj.h>

static char * elix_os_font_defaults[8] = {
	"Serif", "times.ttf",
	"Sans-Serif", "arial.ttf",
	"Monospace", "consola.ttf",
	"", "seguiemj.ttf"
};

//TODO: Look up HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts
elix_databuffer elix_os_font(const char * font_name ) {
	elix_databuffer font_data = {0,nullptr};
	char font_filename[ELIX_FILE_PATH_LENGTH]= {0};

	if ( SHGetFolderPath(nullptr, CSIDL_FONTS, nullptr, 0, font_filename) == S_OK) {
		elix_cstring_append(font_filename, ELIX_FILE_PATH_LENGTH, "\\", 1 );
		if (!font_name) {
			elix_cstring_append(font_filename, ELIX_FILE_PATH_LENGTH, elix_os_font_defaults[7], elix_cstring_length(elix_os_font_defaults[7]) );
		} else {
			for (uint8_t var = 0; var < 8; var += 2) {
				if ( var == 6 ) {
					elix_cstring_append(font_filename, ELIX_FILE_PATH_LENGTH, elix_os_font_defaults[7], elix_cstring_length(elix_os_font_defaults[7]) );
				} else if ( elix_cstring_has_suffix(font_name,elix_os_font_defaults[var]) ) {
					elix_cstring_append(font_filename, ELIX_FILE_PATH_LENGTH, elix_os_font_defaults[var+1], elix_cstring_length(elix_os_font_defaults[var+1]) );
					var = 8;
				}
			}

		}
		elix_file font_file;
		if ( elix_file_open(&font_file, font_filename)) {
			font_data.data = elix_file_read_content(&font_file, font_data.size );
			elix_file_close(&font_file);
		}

	}

	return font_data;

}


#endif // ELIX_SYSINFO_HPP
#endif