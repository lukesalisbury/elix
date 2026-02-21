#ifdef PLATFORM_LINUX
#ifndef ELIX_OS_LINUX_HPP
#define ELIX_OS_LINUX_HPP

#include "elix_os.h"
#include "elix_cstring.h"
#include "elix_file.h"

#include <unistd.h>
#include <stdlib.h>


char * elix_program_directory__special_get(ELIX_PROGRAM_DIRECTORY dir_type, const char * folder_name, UNUSEDARG size_t extra_bytes) {
	static const char * specials[ELIX_PROGRAM_DIRECTORY_LENGTH] = {
		"XDG_DATA_DIRS", //EPDS_DOCS_PUBLIC
		"XDG_DATA_DIRS", //EPDS_DOCS_PRIVATE
		"XDG_DATA_HOME", //EPDS_USER_ROAMING
		"XDG_CACHE_HOME" //EPDS_USER_LOCAL
	};
	static const char * failsafe_path[ELIX_PROGRAM_DIRECTORY_LENGTH] = {
		"/usr/share",
		"/usr/share",
		"~/.local/share",
		"~/.cache"
	};
	ASSERT(dir_type < ELIX_PROGRAM_DIRECTORY_LENGTH);

	char * directory = ALLOCATE(char, ELIX_FILE_PATH_LENGTH);
	char * env_path = getenv( specials[dir_type]); /* DON'T MODIFY */

	if ( env_path ) {
		char buffer_directory[ELIX_FILE_PATH_LENGTH] = {0};
		char ** dir_list = elix_cstring_split(env_path, ':', 0);
		//Loop through env list, to see if directory exist in it
		if ( dir_list ) {
			size_t in = 0;
			while (dir_list[in]) {
				for (size_t c = 0; c < ELIX_FILE_PATH_LENGTH;  c++ ) {
					buffer_directory[c] = 0;
				}
				elix_cstring_append(buffer_directory, ELIX_FILE_PATH_LENGTH, dir_list[in], elix_cstring_length(dir_list[in], 0));
				elix_cstring_append(buffer_directory, ELIX_FILE_PATH_LENGTH, "/", 1);
				elix_cstring_append(buffer_directory, ELIX_FILE_PATH_LENGTH, folder_name, elix_cstring_length(folder_name, 0));

				if ( elix_os_directory_is(buffer_directory, nullptr) ) {
					elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, dir_list[in], elix_cstring_length(dir_list[in], 0));
					in = SIZE_MAX;
				} else {
					in++;
				}
			}
			//If directory is empty use last item in list
			if ( !directory[0] ) {
				elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, dir_list[in - 1], elix_cstring_length(dir_list[in-1], 0));
			}
			free(dir_list);
		}
	} else {
		if ( failsafe_path[dir_type][0] == '~' ) {
			char * home_path = getenv("HOME");/* DON'T MODIFY */
			if ( home_path ) {
				elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, home_path, elix_cstring_length(home_path, 0));
				elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, failsafe_path[dir_type]+1, elix_cstring_length(failsafe_path[dir_type]+1, 0));
			} else {
				elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, "/tmp/", 5);
			}
		} else {
			elix_cstring_append(directory, ELIX_FILE_PATH_LENGTH, failsafe_path[dir_type], elix_cstring_length(failsafe_path[dir_type], 0));
		}

	}

	return directory;
}

size_t elix_os_memory_usage() {
	size_t rss = 0;
	FILE* fp = nullptr;
	if ( (fp = fopen( "/proc/self/statm", "r" )) != nullptr ) {
		fscanf( fp, "%*s%zd", &rss );
	}
	fclose( fp );
	return rss * (size_t)(sysconf( _SC_PAGESIZE));

}
elix_databuffer elix_os_font(const char * font_name ) {
	elix_databuffer font_data;
	char requested_filename[255];
	char command_line[64] = "fc-match -f %{file} ";
	if ( !font_name ) {
		elix_cstring_append(command_line, 64, "emoji", 5);
	} else {
		char * sanitise_font_name = elix_cstring_from(font_name, "sans-serif", 32);
		/* We are writing to the command line, we sanitise the font name,
		   but this remove spaces, so that need to be fixed. */
		elix_cstring_sanitise(sanitise_font_name);
		elix_cstring_append(command_line, 64, sanitise_font_name, elix_cstring_length(sanitise_font_name, 0));
		NULLIFY(sanitise_font_name);
	}


	FILE * cmd_output;
	if ( (cmd_output = popen(command_line, "r")) ) {
		fgets(requested_filename, 255, cmd_output);
		pclose(cmd_output);
	}

	LOG_MESSAGE("File requested: %s %s", font_name,requested_filename);

	elix_file font_file;
	if ( elix_file_open(&font_file, requested_filename, EFF_FILE_READ, nullptr) ) {
		font_data.data = elix_file_read_content(&font_file, &font_data.size );
		elix_file_close(&font_file);
	}

	return font_data;
}

elix_file * elix_platform_font( char * font_name ) {

	char requested_filename[255];
	char command_line[64] = "fc-match -f %{file} ";
	if ( font_name ) {
		elix_cstring_sanitise(font_name);
	}
	elix_cstring_append(command_line, 64, font_name ? font_name : "emoji", font_name ? elix_cstring_length(font_name, 0): 5);

	FILE * cmd_output;
	if ( (cmd_output = popen(command_line, "r")) ) {
		fgets(requested_filename, 255, cmd_output);
		pclose(cmd_output);
	}

	LOG_MESSAGE("File requested: %s %s", font_name,requested_filename);
	return elix_file_new(requested_filename);
}


void elix_os_system_idle(uint32_t time) {
	
}

#endif
#endif
