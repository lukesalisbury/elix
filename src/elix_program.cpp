/***********************************************************************************************************************
 Copyright (c) Luke Salisbury
 This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
 liable for any damages arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
 it and redistribute it freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
	If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is
	not required.
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
	software.
 3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/

#include "elix_core.h"
#include "elix_cstring.hpp"
#include "elix_program.hpp"
#include "elix_os.hpp"

#ifndef MAX_PATH
	#define MAX_PATH 512
#endif

elix_path elix_path_create(const char * string) {
	elix_path uri;
	size_t length = elix_cstring_length(string);
	size_t split = SIZE_MAX;
	size_t extension_split = SIZE_MAX;
	for (split = length-1; split > 0; split--) {
		if ( string[split] == '\\' || string[split] == '/') {
			split++;
			break;
		}
		if ( extension_split == SIZE_MAX && string[split] == '.') {
			extension_split = split +1;
		}
	}

	ASSERT(split < length);
	if ( split == 0 ) { // Default if no path found
		uri.path = "./";
	} else {
		uri.path = elix_cstring_from(string, "./", split );
	}
#ifdef PLATFORM_WINDOWS
	elix_cstring_char_replace(uri.path, '\\', '/');
#endif
	if ( extension_split != SIZE_MAX ) {
		uri.filename = elix_cstring_from(string + split, "/", extension_split - split);
		uri.filetype = elix_cstring_from(string + extension_split, "/");
	} else {
		uri.filename = elix_cstring_from(string + split, "/", length - split);
		uri.filetype = nullptr;
	}

	return uri;
}


elix_program_info elix_program_info_create(const char* arg0 , const  char * program_name, const  char * version, const  char * version_level) {
	elix_program_info info;
	ssize_t dir_len = 0;

	info.user = "TODO";

	info.program_name = elix_cstring_from(program_name, "elix_example");
	info.program_version = elix_cstring_from(version, "Testing");
	if ( version_level == nullptr ) {
		info.program_version_level = nullptr;
		info.program_directory = elix_cstring_from(program_name , "elix_unknown_program");
	} else {
		info.program_version_level = elix_cstring_from(version_level, "1");
		info.program_directory = nullptr;

		dir_len = snprintf ( nullptr, 0, "%s-%s", info.program_name, info.program_version_level );
		if ( dir_len ) {
			info.program_directory = new char[dir_len+1]();
			snprintf ( info.program_directory, dir_len, "%s-%s", info.program_name, info.program_version_level );
		} else {
			info.program_directory = elix_cstring_from(program_name , "elix_unknown_program");
		}
	}

	elix_cstring_sanitise(info.program_directory);

	info.path_executable = elix_path_create(arg0);


	return info;
}




enum ELIX_PROGRAM_DIRECTORY {
	EPDS_DOCS_PUBLIC,
	EPDS_DOCS_PRIVATE,
	EPDS_USER_ROAMING,
	EPDS_USER_LOCAL,
	ELIX_PROGRAM_DIRECTORY_LENGTH
};

#ifdef PLATFORM_LINUX
#include <stdlib.h>

char * elix_program_directory__special_get(ELIX_PROGRAM_DIRECTORY dir_type, UNUSEDARG size_t extra_bytes) {
	static const char * specials[ELIX_PROGRAM_DIRECTORY_LENGTH] = {
		"XDG_DATA_DIRS",
		"XDG_DATA_DIRS",
		"XDG_DATA_HOME",
		"XDG_CACHE_HOME"
	};
	static const char * failsafe_path[ELIX_PROGRAM_DIRECTORY_LENGTH] = {
		"/usr/share",
		"/usr/share",
		"~/.local/share",
		"~/.cache"
	};
	ASSERT(dir_type < ELIX_PROGRAM_DIRECTORY_LENGTH);

	char * directory = new char[MAX_PATH]();
	char * env_path = getenv( specials[dir_type]); /* DON'T MODIFY */
	if ( env_path ) {
		elix_cstring_append(directory, MAX_PATH, env_path, elix_cstring_length(env_path));
	} else {
		if ( failsafe_path[dir_type][0] == '~' ) {
			char * home_path = getenv("HOME");/* DON'T MODIFY */
			if ( home_path ) {
				elix_cstring_append(directory, MAX_PATH, home_path, elix_cstring_length(home_path));
				elix_cstring_append(directory, MAX_PATH, failsafe_path[dir_type]+1, elix_cstring_length(failsafe_path[dir_type]+1));
			} else {
				elix_cstring_append(directory, MAX_PATH, "/tmp/", 5);
			}
		} else {
			elix_cstring_append(directory, MAX_PATH, failsafe_path[dir_type], elix_cstring_length(failsafe_path[dir_type]));
		}

	}

	return directory;
}
#endif


#ifdef PLATFORM_WINDOWS

#ifndef _WIN32_IE
	#define _WIN32_IE 0x0600
#endif
#include <io.h>
#include <shlobj.h>
#include <direct.h>

char * elix_program_directory__special_get(ELIX_PROGRAM_DIRECTORY dir_type, UNUSEDARG size_t extra_bytes) {
	static int32_t specials[ELIX_PROGRAM_DIRECTORY_LENGTH] = {
		CSIDL_COMMON_DOCUMENTS,
		CSIDL_PERSONAL,
		CSIDL_APPDATA,
		CSIDL_LOCAL_APPDATA
	};
	ASSERT(dir_type < ELIX_PROGRAM_DIRECTORY_LENGTH);

	char * directory = new char[MAX_PATH]();
	if ( SHGetSpecialFolderPath(NULL, directory, specials[dir_type], 1) ) {
		elix_cstring_char_replace(directory, '\\', '/');
	} else {
		elix_cstring_append(directory, MAX_PATH, "c:/temp/", 8);
	}
	return directory;
}

#endif

inline void elix_path__real(char * path, size_t len) {

	size_t from = SIZE_MAX, to = 0, skip_sep = 0;
	for (size_t c = len; c > 0; c--) {
		if ( from == SIZE_MAX ) {
			if (path[c] == '/') {
				from = c;
				to = SIZE_MAX;
			}
		} else if ( skip_sep >= 3 ) {
			from = SIZE_MAX;
			skip_sep = 0;
		} else {
			if (path[c] == '.') {
				skip_sep++;
			} else if (path[c] == '/') {
				skip_sep--;
				if ( skip_sep == 0 ) {
					to = c;
					memmove(path + to, path + from, len+1 - from );
					from = SIZE_MAX;
					skip_sep = 0;
				}
			} else if ( skip_sep  == 0) {
				from = SIZE_MAX;
				skip_sep = 0;
			}
		}


	}

}


char * elix_program_directory_documents( const elix_program_info * program_info, bool shared, const char * filename ){
	ASSERT(program_info != nullptr);

	uint8_t errors = 0;
	size_t name_len = elix_cstring_length(filename);
	size_t sub_len = elix_cstring_length(program_info->program_directory);
	char * directory = nullptr;

	directory = elix_program_directory__special_get((shared ? EPDS_DOCS_PUBLIC : EPDS_DOCS_PRIVATE), name_len+sub_len+2);

	errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
	errors += elix_cstring_append(directory, MAX_PATH, program_info->program_directory, sub_len);

	if ( filename ) {
		errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
		errors += elix_cstring_append(directory, MAX_PATH, filename, name_len);
	}

	if ( errors ) {
		delete [] directory;
		return nullptr;
	} else {
		return directory;
	}

}

char * elix_program_directory_user( const elix_program_info * program_info, bool roaming, const char * filename ) {
	ASSERT(program_info != nullptr);

	uint8_t errors = 0;
	size_t name_len = elix_cstring_length(filename);
	size_t sub_len = elix_cstring_length(program_info->program_directory);
	char * directory = nullptr;

	directory = elix_program_directory__special_get((roaming ? EPDS_USER_ROAMING : EPDS_USER_LOCAL), name_len+sub_len+2);

	errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
	errors += elix_cstring_append(directory, MAX_PATH, program_info->program_directory, sub_len);

	if ( filename ) {
		errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
		errors += elix_cstring_append(directory, MAX_PATH, filename, name_len);
	}

	if ( errors ) {
		delete [] directory;
		return nullptr;
	} else {
		return directory;
	}

}


char * elix_program_directory_resources( const elix_program_info * program_info, const char * filename, UNUSEDARG uint8_t override_lookup ) {
	ASSERT(program_info != nullptr);

	uint8_t errors = 0;
	size_t dir_len = 0, name_len = 0;
	char * directory = nullptr;

	if ( filename != nullptr ) {
		name_len = elix_cstring_length(filename);
	}

	if ( program_info->resource_directory_type == EPRD_DATA || program_info->resource_directory_type == EPRD_AUTO ) {
		dir_len = snprintf ( nullptr, 0, "%s/%s_data", program_info->path_executable.path, program_info->path_executable.filename );
		directory = new char[dir_len+name_len+1]();
		snprintf ( directory, dir_len, "%s/%s_data", program_info->path_executable.path, program_info->path_executable.filename );
		if ( elix_os_directory_is(directory)) {
			if ( filename != nullptr ) {
				errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
				errors += elix_cstring_append(directory, dir_len+name_len, filename, name_len);
			}
			return directory;
		}
		delete [] directory;
	}

	if ( program_info->resource_directory_type == EPRD_SHARE_IN_PARENT || program_info->resource_directory_type == EPRD_AUTO ) {
		dir_len = snprintf ( nullptr, 0, "%s/../share/%s", program_info->path_executable.path, program_info->program_directory );
		directory = new char[dir_len+name_len+1]();
		snprintf ( directory, dir_len, "%s/../share/%s", program_info->path_executable.path, program_info->program_directory );
		elix_path__real(directory, dir_len);
		if ( elix_os_directory_is(directory) ) {
			if ( filename != nullptr ) {
				errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
				errors += elix_cstring_append(directory, dir_len+name_len, filename, name_len);
			}
			return directory;
		}
		delete [] directory;
	}

	if ( program_info->resource_directory_type == EPRD_SHARE || program_info->resource_directory_type == EPRD_AUTO ) {
		dir_len = snprintf ( nullptr, 0, "%s/share/%s", program_info->path_executable.path, program_info->program_directory );
		directory = new char[dir_len+name_len+1]();
		snprintf ( directory, dir_len, "%s/share/%s", program_info->path_executable.path, program_info->program_directory );
		if ( elix_os_directory_is(directory)) {
			if ( filename != nullptr ) {
				errors += elix_cstring_append(directory, MAX_PATH, "/", 1);
				errors += elix_cstring_append(directory, dir_len+name_len, filename, name_len);
			}
			return directory;
		}
		delete [] directory;
	}
	return nullptr;
}

char * elix_program_directory_cache_file( const elix_program_info * program_info, const char * filename) {

	return elix_program_directory_user(program_info, true, filename);
}

