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



/*
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

	uri.uri = elix_cstring_from( string );
	uri.buffer = elix_cstring_from( string );
	uri.path = uri.buffer + split;

#ifdef PLATFORM_WINDOWS
	elix_cstring_char_replace(uri.buffer, '\\', '/');
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
*/

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
		uri.path = elix_cstring_from(string, "./", split );
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


char * elix_program_directory__special_get(ELIX_PROGRAM_DIRECTORY dir_type, const char * folder_name, UNUSEDARG size_t extra_bytes);


elix_program_info elix_program_info_create(const char* arg0 , const  char * program_name, const  char * version, const  char * version_level) {
	elix_program_info info;
	ssize_t dir_len = 0;

	elix_cstring_copy("TODO", info.user, 16);
	// GetUserNameEx(NameDisplay, info.user, nullptr);
	elix_cstring_copy(getenv("USER"), info.user, 16);

	info.program_name = elix_cstring_from(program_name, "elix_example", 64);
	info.program_version = elix_cstring_from(version, "Testing", 64);
	if ( version_level == nullptr ) {
		info.program_version_level = nullptr;
		info.program_directory = elix_cstring_from(program_name , "elix_unknown_program", 64);
	} else {
		info.program_version_level = elix_cstring_from(version_level, "1", 64);
		info.program_directory = nullptr;

		dir_len = snprintf ( nullptr, 0, "%s-%s", info.program_name, info.program_version_level );
		if ( dir_len ) {
			info.program_directory = ALLOCATE(char, dir_len+1);
			snprintf ( info.program_directory, dir_len, "%s-%s", info.program_name, info.program_version_level );
		} else {
			info.program_directory = elix_cstring_from(program_name , "elix_unknown_program", 64);
		}
	}

	elix_cstring_sanitise(info.program_directory);

	info.path_executable = elix_path_create(arg0);


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


char * elix_program_directory_documents( const elix_program_info * program_info, bool shared, const char * filename ){
	ASSERT(program_info != nullptr);

	uint8_t errors = 0;
	size_t name_len = elix_cstring_length(filename, 0);
	size_t sub_len = elix_cstring_length(program_info->program_directory, 0);
	char * directory = nullptr;

	directory = elix_program_directory__special_get((shared ? EPDS_DOCS_PUBLIC : EPDS_DOCS_PRIVATE), program_info->program_directory, name_len+sub_len+2);

	errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
	errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, program_info->program_directory, sub_len);

	if ( filename ) {
		errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
		errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, filename, name_len);
	}

	if ( errors ) {
		NULLIFY(directory);
		return nullptr;
	} else {
		return directory;
	}

}

char * elix_program_directory_user( const elix_program_info * program_info, bool roaming, const char * filename ) {
	ASSERT(program_info != nullptr);

	uint8_t errors = 0;
	size_t name_len = elix_cstring_length(filename, 0);
	size_t sub_len = elix_cstring_length(program_info->program_directory, 0);
	char * directory = nullptr;

	directory = elix_program_directory__special_get((roaming ? EPDS_USER_ROAMING : EPDS_USER_LOCAL), program_info->program_directory, name_len+sub_len+2);

	errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
	errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, program_info->program_directory, sub_len);

	if ( filename ) {
		errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
		errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, filename, name_len);
	}

	if ( errors ) {
		NULLIFY(directory);
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
		name_len = elix_cstring_length(filename,0);
	}

	if ( program_info->resource_directory_type == EPRD_DATA || program_info->resource_directory_type == EPRD_AUTO ) {
		dir_len = snprintf ( nullptr, 0, "%s/%s_data", program_info->path_executable.path, program_info->path_executable.filename );
		dir_len += name_len + 2;
		directory = ALLOCATE(char, dir_len);
		snprintf ( directory, dir_len, "%s/%s_data", program_info->path_executable.path, program_info->path_executable.filename );
		if ( elix_os_directory_is(directory, nullptr)) {
			if ( filename != nullptr ) {
				errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
				errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, filename, name_len);
			}
			return directory;
		}
		NULLIFY(directory)
	}

	if ( program_info->resource_directory_type == EPRD_SHARE_IN_PARENT || program_info->resource_directory_type == EPRD_AUTO ) {
		dir_len = snprintf ( nullptr, 0, "%s/../share/%s", program_info->path_executable.path, program_info->program_directory );
		dir_len += name_len + 2;
		directory = ALLOCATE(char, dir_len);
		snprintf ( directory, dir_len, "%s/../share/%s", program_info->path_executable.path, program_info->program_directory );
		elix_path__real(directory, dir_len);
		if ( elix_os_directory_is(directory, nullptr) ) {
			if ( filename != nullptr ) {
				errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
				errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, filename, name_len);
			}
			return directory;
		}
		NULLIFY(directory)

	}

	if ( program_info->resource_directory_type == EPRD_SHARE || program_info->resource_directory_type == EPRD_AUTO ) {
		dir_len = snprintf ( nullptr, 0, "%s/share/%s", program_info->path_executable.path, program_info->program_directory );
		dir_len += name_len + 2;
		directory = ALLOCATE(char, dir_len);
		snprintf ( directory, dir_len, "%s/share/%s", program_info->path_executable.path, program_info->program_directory );
		if ( elix_os_directory_is(directory, nullptr)) {
			if ( filename != nullptr ) {
				errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
				errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, filename, name_len);
			}
			return directory;
		}
		NULLIFY(directory)
	}

	if ( program_info->resource_directory_type == EPRD_GLOBAL || program_info->resource_directory_type == EPRD_AUTO ) {
		//TODO: Remove hardcode path
		dir_len = snprintf ( nullptr, 0, "/usr/share/%s", program_info->program_directory );
		dir_len += name_len + 2;
		directory = ALLOCATE(char, dir_len);
		snprintf ( directory, dir_len, "/usr/share/%s", program_info->program_directory );
		if ( filename != nullptr ) {
			errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/", 1);
			errors += elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, filename, name_len);
		}
		NULLIFY(directory)
	}
	return nullptr;
}

char * elix_program_directory_cache_file( const elix_program_info * program_info, const char * filename) {

	return elix_program_directory_user(program_info, true, filename);
}


