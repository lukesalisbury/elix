#ifndef ELIX_OS_LINUX_HPP
#define ELIX_OS_LINUX_HPP

#include "elix_core.h"
#include "elix_cstring.hpp"
#include "elix_file.hpp"
#include <unistd.h>

size_t elix_os_memory_usage() {
	size_t rss = 0;
	FILE* fp = nullptr;
	if ( (fp = fopen( "/proc/self/statm", "r" )) != nullptr ) {
		fscanf( fp, "%*s%zd", &rss );
	}
	fclose( fp );
	return rss * static_cast<size_t>(sysconf( _SC_PAGESIZE));

}
elix_databuffer elix_os_font(const char * font_name ) {
	elix_databuffer font_data;
	char requested_filename[255];
	char command_line[64] = "fc-match -f %{file} ";
	if ( !font_name ) {
		elix_cstring_append(command_line, 64, "emoji", 5);
	} else {
		char * sanitise_font_name = elix_cstring_from(font_name, "sans-serif");
		/* We are writing to the command line, we sanitise the font name,
		   but this remove spaces, so that need to be fixed. */
		elix_cstring_sanitise(sanitise_font_name);
		elix_cstring_append(command_line, 64, sanitise_font_name, elix_cstring_length(sanitise_font_name, 0));
		delete sanitise_font_name;
	}


	FILE * cmd_output;
	if ( (cmd_output = popen(command_line, "r")) ) {
		fgets(requested_filename, 255, cmd_output);
		pclose(cmd_output);
	}

	LOG_MESSAGE("File requested: %s %s", font_name,requested_filename);

	elix_file font_file;
	if ( elix_file_open(&font_file, requested_filename)) {
		font_data.data = elix_file_read_content(&font_file, font_data.size );
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


void elix_os_system_idle(uint32_t time)
{
	
}

#endif
