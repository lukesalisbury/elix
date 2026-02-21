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
#include <stdarg.h>

#if __WIN32__
#include <process.h>
#else
#include <spawn.h>
#endif

void elix_os_command_capture_unsafe( const char * program_and_arguments, char * buffer, size_t buffer_size, UNUSEDARG char ** env) {
	int err = 0;
	size_t count = 0;
	size_t str_size = 0;
	size_t str_position = 0;


	str_size = elix_cstring_length(program_and_arguments, 1);

	if ( str_size > 0x1FF ) {
		LOG_INFO("Command length is to much: %s", program_and_arguments);
		return;
	}


	FILE * command_pipe = popen(program_and_arguments, "r");
	if ( !command_pipe ) {
		LOG_INFO("Error running command: %s", program_and_arguments);
	} else {
		size_t q = fread(buffer, buffer_size, 1, command_pipe);
		pclose(command_pipe);
	}

}


void elix_os_command_capture( char * program, char * buffer, size_t buffer_size, char ** env, ...) {
	int err = 0;
	size_t count = 0;
	size_t str_size = 0;
	size_t str_position = 0;

	char * command_string = nullptr;

	va_list passed_arguments, count_arguments;
	va_start(passed_arguments, env);
	va_copy(count_arguments, passed_arguments);
	while ( true ) {
		char * carg = va_arg(count_arguments, char*);
		if (!carg) break;
		str_size += elix_cstring_length(carg, 1);
		count++;
	}
	va_end(count_arguments);

	str_size += elix_cstring_length(program, 1) + count;

	if ( str_size > 0x1FF ) {
		LOG_INFO("Command lenght is to much: %s", program);
		return;
	}

	command_string = (char*) calloc(str_size, sizeof(char));

	str_position = elix_cstring_append(command_string, str_size, program, elix_cstring_length(program, 1) );

	if ( count ) {
		for ( size_t i = 1; i <= count; i++) {
			if ( str_position < str_size ) {
				command_string[str_position-1] = ' ';
			}
			char * current_arg = va_arg(passed_arguments, char*);
			str_position = elix_cstring_append(command_string, str_size, current_arg, elix_cstring_length(current_arg, 1) );
		}
	}


	FILE * command_pipe = popen(command_string, "r");
	if ( !command_pipe ) {
		LOG_INFO("Error running command: %s", program);
	} else {
		size_t q = fread(buffer, buffer_size, 1, command_pipe);
		pclose(command_pipe);
	}

	va_end(passed_arguments);

	NULLIFY(command_string);

}

void elix_os_command( char * program, char ** env, ...) {
//
	int err = 0;
	size_t count = 0;
	size_t str_size = 0;
	char ** arguments = nullptr;

	va_list passed_arguments, count_arguments;
	va_start(passed_arguments, env);
	va_copy(count_arguments, passed_arguments);
	while ( true ) {
		char * carg = va_arg(count_arguments, char*);
		if (!carg) break;
		count++;
	}
	va_end(count_arguments);

	if ( count ) {
		arguments = (char**)calloc((count + 2) , sizeof(char *));
		arguments[0] = program;
		for ( size_t i = 1; i <= count; i++) {
			arguments[i]  = va_arg(passed_arguments, char*);
		}
	}

	#ifdef __WIN32__
	err = _spawnvp( _P_WAIT, arguments[0], (const char *const *) arguments );
	#else
	pid_t pid;
	err = posix_spawnp(&pid, arguments[0], nullptr, nullptr, arguments, nullptr );
	#endif
	
	if ( err != 0 ) {
		LOG_INFO("Error running command: %s", program);
	}

	va_end(passed_arguments);

	NULLIFY(arguments);

}
