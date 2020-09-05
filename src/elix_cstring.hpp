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

#ifndef ELIX_CSTRING_HPP
#define ELIX_CSTRING_HPP

#include "elix_core.h"

#ifndef SSIZE_MAX
#ifdef _WIN64
#define SSIZE_MAX INT64_MAX
#define SSIZE_MIN INT64_MIN
#else
#define SSIZE_MAX INT32_MAX
#define SSIZE_MIN INT32_MIN
#endif
#endif

inline uint32_t elix_hash( const char * str, size_t len ) {
	//Jenkins One-at-a-time hash
	uint32_t hash = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		hash += str[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

inline size_t elix_ucstring_length(const uint8_t * string, uint8_t include_terminator = 0 ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}
inline size_t elix_cstring_length(const char * string, uint8_t include_terminator = 0 ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}

bool elix_cstring_has_suffix( const char * str, const char * suffix );
bool elix_cstring_has_prefix( const char * str, const char * prefix );
size_t elix_cstring_find_not_of( const char * str, const char * search, size_t offset = 0 );
size_t elix_cstring_find_of( const char * str, const char * search, size_t offset = 0 );
void  elix_cstring_sanitise( char * string );
size_t elix_cstring_trim( char * string );

char * elix_cstring_substr( const char * source, ssize_t pos = 0, ssize_t len = SSIZE_MAX );
char * elix_cstring_from( const char * source, const char * default_str, size_t max_length = SIZE_MAX );
char * elix_cstring_copy( const char * source, size_t max_length = SIZE_MAX );

void elix_cstring_char_replace( char * str, const char search, const char replace);
uint8_t elix_cstring_append( char * str, const size_t len, const char * text, const size_t text_len);

uint32_t elix_cstring_next_character(char *& object);
uint32_t elix_cstring_peek_character(char * object);


#endif // ELIX_CSTRING_HPP
