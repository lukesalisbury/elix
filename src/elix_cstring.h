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
#include <sys/types.h>

#define ELIX_CHAR_LOWER 1
#define ELIX_CHAR_UPPER 2
#define ELIX_CHAR_CAPITALISE 3


static inline uint32_t elix_hash( const char * str, size_t len ) {
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


#ifdef __cplusplus
extern "C" {
#endif

void elix_cstring_clear( char * string, const size_t len );

bool elix_cstring_has_suffix( const char * str, const char * suffix );
bool elix_cstring_has_prefix( const char * str, const char * prefix );

void  elix_cstring_sanitise( char * string );
size_t elix_cstring_trim( char * string );


size_t elix_ucstring_length(const uint8_t * string, uint8_t include_terminator );
size_t elix_cstring_length(const char * string, uint8_t include_terminator);
size_t elix_cstring_find_not_of( const char * str, const char * search, size_t offset );
size_t elix_cstring_find_of( const char * str, const char * search, size_t offset );

char * elix_cstring_substr( const char * source, ssize_t pos, ssize_t len );
char * elix_cstring_from( const char * source, const char * default_str, size_t max_length );

void elix_cstring_copy( const char * source_init, char * dest_init, size_t max_length);




void elix_cstring_char_replace( char * str, const char search, const char replace);
uint8_t elix_cstring_append( char * str, const size_t len, const char * text, const size_t text_len);

uint32_t elix_cstring_next_character(char * object);
uint32_t elix_cstring_peek_character(char * object);

char ** elix_cstring_split( const char * source, char token, char string_bracket);

#ifdef __cplusplus
}
#endif

inline char * elix_cstring_new( const char * source ) {
	return elix_cstring_from( nullptr, source, elix_cstring_length(source, 0) );
}


// Default parameters functions

inline size_t elix_ucstring_length_stupid_c(const uint8_t * string) {
	return elix_ucstring_length(string, 0);
}
inline size_t elix_cstring_length_stupid_c(const char * string) {
	return elix_cstring_length(string, 0);
}
inline size_t elix_cstring_find_not_of_stupid_c( const char * str, const char * search ) {
	return elix_cstring_find_not_of( str, search, 0);
}
inline size_t elix_cstring_find_of_stupid_c( const char * str, const char * search ) {
	return elix_cstring_find_of( str, search, 0);
}
inline char * elix_cstring_from_stupid_c( const char * source, const char * default_str) {
	return elix_cstring_from( source, default_str, SIZE_MAX);
}
inline void elix_cstring_copy_stupid_c( const char * source_init, char * dest_init) {
	elix_cstring_copy( source_init, dest_init, SIZE_MAX);
}


#endif // ELIX_CSTRING_HPP
