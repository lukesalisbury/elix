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

#include "elix_os.h"
#include "elix_cstring.h"
#include <string.h>
#include <stdlib.h>

#if defined PLATFORM_WINDOWS
	//#include "elix_os_directory_win.cpp"
	//#include "elix_os_win.cpp"
#elif defined PLATFORM_LINUX
	#define ELIX_USE_DIRECTORY_POSIX 1
	//#include "elix_os_directory_posix.cpp"
	//#include "elix_os_linux.cpp"
#elif defined PLATFORM_3DS
	//#include "elix_os_directory_posix.cpp"
	#define ELIX_USE_DIRECTORY_POSIX 1
#elif defined PLATFORM_NXSWITCH
	//#include "elix_os_directory_posix.cpp"
	#define ELIX_USE_DIRECTORY_POSIX 1
#elif defined PLATFORM_SDL2
	//#include "elix_os_directory_posix.cpp"
	#define ELIX_USE_DIRECTORY_POSIX 1
#else
	#error "Unsupported platform"
#endif

/* elix_program_directory__special_get is stored in elix_os_directory_*.c */
char * elix_program_directory__special_get(elix_program_special_directory dir_type, const char * folder_name, UNUSEDARG size_t extra_bytes);

elix_path elix_path_create(const char * string) {
	elix_path uri = { nullptr, nullptr, nullptr, nullptr};
	
	size_t length = elix_cstring_length(string, 0);
	size_t split = SIZE_MAX;
	size_t extension_split = SIZE_MAX;
	for (split = length-1; split > 0; split--) {
		if ( string[split] == '\\' || string[split] == '/') {
			split++;
			break;
		}
		if ( extension_split == SIZE_MAX && string[split] == '.') {
			extension_split = split;
		}
	}

	uri.uri = (char *)calloc(1, length+1);

	elix_cstring_copy(string, uri.uri, length+1);
	//uri.path = elix_cstring_from( string, "/", split );
	if ( split == 0 ) { // Default if no path found
		uri.path = elix_cstring_from(nullptr, "./", 2 );
	} else {
		uri.path = elix_cstring_from(string, "./", split + 1 );
	}

#ifdef PLATFORM_WINDOWS
	elix_cstring_char_replace(uri.path, '\\', '/');
#endif
	if ( extension_split != SIZE_MAX && extension_split != split ) {
		uri.filename = elix_cstring_from(string + split, "/", extension_split - split + 1);
		uri.filetype = elix_cstring_from(string + extension_split, "/",SIZE_MAX);
	} else {
		uri.filename = elix_cstring_from(string + split, "/", length - split+1);
		uri.filetype = nullptr;
	}
	return uri;
}

void elix_path_destroy(elix_path * path) {
	if (!path) {
		return;
	}

	NULLIFY(path->filename);
	NULLIFY(path->filetype);
	NULLIFY(path->path);
	NULLIFY(path->uri);
}


elix_program_info elix_program_info_create(const char * arg0, const char * program_name, const char * version_release, const char * version_directory) {
	elix_program_info info = {0, .system_user = "Unknown"};
	ssize_t dir_len = 0;

	#if ELIX_USE_DIRECTORY_POSIX
		elix_cstring_copy(getenv("USER"), info.system_user, 15);
	#else
		elix_cstring_copy("Windows", info.system_user, 15);
	#endif

	elix_cstring_copy(program_name, info.program_name, 31);
	elix_cstring_copy(version_release, info.program_version, 7);
	elix_cstring_copy(version_directory, info.program_version_directory, 15);

	elix_cstring_append(info.program_directory, 40, info.program_name, 31);
	
	if ( version_directory != nullptr ) {
		elix_cstring_append(info.program_directory, 40, "-", 1);
		elix_cstring_append(info.program_directory, 40, info.program_version_directory, 7);
	}

	elix_cstring_sanitise(info.program_directory);

	info.path_executable = elix_path_create(arg0);

	elix_cstring_append(info.program_executable, 40, info.path_executable.filename, elix_cstring_length(info.path_executable.filename, 1) );
	elix_cstring_sanitise(info.program_executable);

	return info;
}


extern inline void elix_path__real(char * path, size_t len) {
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

function_results elix_program_directory_create(const elix_program_info * program_info, elix_program_special_directory directory) {
	ASSERT(program_info != nullptr);

	if ( directory < EPSD_USER_CONFIG ) {
		LOG_ERROR("System directories can not be created");
		return RESULTS_FAILED;
	}

	char * requested_folder = elix_program_directory__special_get(directory, program_info->program_directory, 0);

	if ( !elix_os_directory_is(requested_folder, nullptr) ) {
		if ( elix_os_directory_make(requested_folder, 744, false, nullptr) ) {
			return RESULTS_SUCCESS;
		}
		return RESULTS_ERROR;
	}
	return RESULTS_ERROR;
}

char * elix_program_directory_resource__subset(const elix_program_info * program_info, elix_program_resource_directory special, const char * directory, const char * filename ) {
	char * requested_path = nullptr;
	size_t filename_length = elix_cstring_length(filename, 1);

	switch (special) {
		// ../share/"program_name"/
		case EPRD_PARENT_SHARE_MUTATABLE:
		case EPRD_PARENT_SHARE:
			requested_path = elix_cstring_create(program_info->path_executable.path, ELIX_FILE_PATH_LENGTH);
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, "../share/", 10);
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, directory, elix_cstring_length(directory, 0));
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, "/", 1);
			break;
		// ./share/"program_name"/
		case EPRD_SHARE_MUTATABLE:
		case EPRD_SHARE:
			requested_path = elix_cstring_create(program_info->path_executable.path, ELIX_FILE_PATH_LENGTH);
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, "share/", 8);
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, directory, elix_cstring_length(directory, 0));
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, "/", 1);
			break;
		// ./"program_name"_data/
		case EPRD_DATA_MUTATABLE:
		case EPRD_DATA:
			requested_path = elix_cstring_create(program_info->path_executable.path, ELIX_FILE_PATH_LENGTH);
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, directory, elix_cstring_length(directory, 0));
			elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, "_data/", 7);
			break;
		default:
			break;
	}
	//if ( !elix_os_directory_is(requested_path, nullptr) ) {
	//	NULLIFY(requested_path);
	//}

	return requested_path;
}

char * elix_program_directory_resource( const elix_program_info * program_info, const char * filename, uint8_t always_return, elix_program_resource_directory resource_directory )  {
	ASSERT(program_info != nullptr);

	char * requested_path = nullptr;
	size_t filename_length = elix_cstring_length(filename, 1);

	switch ( resource_directory ) {
		case EPRD_AUTO:
		case EPRD_GLOBAL_SHARE: {
			requested_path = elix_program_directory__special_get( EPSD_SYSTEM_RESOURCE, program_info->program_directory, filename_length);
			if ( requested_path != nullptr )
				break;
		}
		case EPRD_LOCAL_SHARE: {
			requested_path = elix_program_directory__special_get( EPSD_USER_RESOURCE, program_info->program_directory, filename_length);
			if ( requested_path != nullptr )
				break;
		}
		case EPRD_PARENT_SHARE:
		case EPRD_SHARE:
		case EPRD_DATA: {
			requested_path = elix_program_directory_resource__subset(program_info, resource_directory, program_info->program_directory, filename);
			if ( requested_path != nullptr )
				break;
		}
		case EPRD_PARENT_SHARE_MUTATABLE:
		case EPRD_SHARE_MUTATABLE:
		case EPRD_DATA_MUTATABLE: {
			requested_path = elix_program_directory_resource__subset(program_info, resource_directory, program_info->path_executable.filename, filename);
			if ( requested_path != nullptr )
				break;
		}
		default:
			break;
	}

	if ( requested_path != nullptr ) {
		if ( elix_os_directory_is(requested_path, nullptr) || always_return ) {
			if ( filename ) {
				elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, filename, filename_length);
			}
			return requested_path;
		}
		NULLIFY(requested_path);
	}

	return requested_path;
}

char * elix_program_directory_config( const elix_program_info * program_info, const char * filename, uint8_t always_return, uint8_t system_version ) {
	ASSERT(program_info != nullptr);

	char * requested_path = nullptr;
	size_t filename_length = elix_cstring_length(filename, 1);

	elix_program_special_directory special = EPSD_USER_CONFIG;
	if ( system_version > 0 ) {
		special = EPSD_SYSTEM_CONFIG;
	}

	requested_path = elix_program_directory__special_get(special, program_info->program_directory, filename_length);
	if ( requested_path != nullptr ) {
		if ( elix_os_directory_is(requested_path, nullptr) || always_return) {
			if ( filename ) {
				elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, filename, filename_length);
			}
			return requested_path;
		}
	}

	NULLIFY(requested_path);
	return requested_path;
}

/* Returns path for User Docs. Such as ~/Documents or CSIDL_PERSONAL on Windows */
char * elix_program_directory_document( const elix_program_info * program_info, const char * filename, uint8_t always_return, uint8_t request_root_directory ) {
	ASSERT(program_info != nullptr);
	
	char * requested_path = nullptr;
	size_t filename_length = elix_cstring_length(filename, 1);

	if ( request_root_directory ) {
		requested_path = elix_program_directory__special_get(EPSD_USER_DOCS, nullptr, filename_length);
	} else {
		requested_path = elix_program_directory__special_get(EPSD_USER_DOCS, program_info->program_directory, filename_length);
	}
	if ( requested_path != nullptr ) {
		if ( elix_os_directory_is(requested_path, nullptr) || always_return ) {
			if ( filename ) {
				elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, filename, filename_length);
			}
			return requested_path;
		}
	}

	NULLIFY(requested_path);
	return requested_path;
}


char * elix_program_directory_cache( const elix_program_info * program_info, const char * filename, uint8_t always_return, uint8_t system_version ) {
	ASSERT(program_info != nullptr);
	
	char * requested_path = nullptr;
	size_t filename_length = elix_cstring_length(filename, 1);

	if ( system_version ) {
		requested_path = elix_program_directory__special_get(EPSD_USER_TEMP, program_info->program_directory, filename_length);
	} else {
		requested_path = elix_program_directory__special_get(EPSD_USER_CACHE, program_info->program_directory, filename_length);
	}

	if ( requested_path != nullptr ) {
		if ( elix_os_directory_is(requested_path, nullptr) || always_return ) {
			if ( filename ) {
				elix_cstring_append(requested_path, ELIX_FILE_PATH_LENGTH, filename, filename_length);
			}
			return requested_path;
		}
	}

	NULLIFY(requested_path);
	return requested_path;
}

