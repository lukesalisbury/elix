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

#include "elix_cstring.hpp"
#include <string.h>

char * elix_cstring_substr( const char * source, ssize_t pos, ssize_t len ) {
	ASSERT(source != nullptr)

	size_t offset = 0;
	size_t max_length = elix_cstring_length(source);
	size_t length = max_length;

	if ( pos < 0 ) {
		offset = length + pos;
	} else {
		offset = pos;
	}

	if ( len != SSIZE_MAX && len != SSIZE_MIN) {
		if ( len < 0 ) {
			length += len;
		} else if ( (size_t)len < length ) {
			length = (size_t)len + offset;
		}
	}

	length -= offset;
	if ( length + offset > max_length ){
		LOG_ERROR("elix_cstring_substr failed, " pZU " + " pZU " > " pZU "", length, offset, max_length);
		return nullptr;
	}

	if ( length < 1 || length > max_length) {
		LOG_ERROR("elix_cstring_substr failed, Length is invalid (" pZU ").", length);
		return nullptr;
	}

	char * dest = new char[length+1]();
	memcpy(dest, source + offset, length);

	return dest;
}

void elix_cstring_char_replace( char * str, const char search, const char replace) {
	size_t length = elix_cstring_length(str, 0);

	for (size_t c = 0; c < length; c++) {
		if ( str[c] == search )
			 str[c] = replace;
	}
}

bool elix_cstring_has_suffix( const char * str, const char * suffix) {
	size_t str_length = elix_cstring_length(str, 0);
	size_t suffix_length = elix_cstring_length(suffix, 0);
	size_t offset = 0;

	if ( str_length < suffix_length )
		return false;

	offset = str_length - suffix_length;
	for (size_t c = 0; c < suffix_length;  c++ ) {
		if ( str[offset + c] != suffix[c] )
			return false;
	}
	return true;
}

bool elix_cstring_has_prefix( const char * str, const char * prefix) {
	size_t str_length = elix_cstring_length(str, 0);
	size_t prefix_length = elix_cstring_length(prefix, 0);

	if ( str_length < prefix_length )
		return false;

	for (size_t c = 0; c < prefix_length;  c++ ) {
		if ( str[c] != prefix[c] )
			return false;
	}
	return true;
}

uint8_t elix_cstring_append( char * str, const size_t len, const char * text, const size_t text_len) {
	size_t length = elix_cstring_length(str, 0);
	// TODO: Switch to memcpy
	for (size_t c = 0;length < len && c < text_len; length++, c++) {
		str[length] = text[c];
	}
	str[length+1] = 0;
	return 0;
}


size_t elix_cstring_find_not_of( const char * str, const char * search, size_t offset) {
	size_t length = elix_cstring_length(str);
	size_t sl = elix_cstring_length(search);
	for (size_t c = offset; c < length && str[c] != 0; c++) {
		bool found = false;
		for (size_t sc = 0; sc <= sl; sc++) {
			if ( str[c] == search[sc]) {
				found = true;
				break;
			}
		}
		if ( !found )
			return c;
	}
	return SIZE_MAX;
}

size_t elix_cstring_find_of(const char * str, const char * search, size_t offset) {
	size_t length = elix_cstring_length(str);
	size_t sl = elix_cstring_length(search);
	for (size_t c = offset; c < length && str[c] != 0; c++) {
		for (size_t sc = 0; sc <= sl; sc++) {
			if ( str[c] == search[sc]) {
				return c;
				break;
			}
		}
	}
	return SIZE_MAX;
}


void elix_cstring_sanitise( char * string ) {
	size_t pos = 0;
	size_t length = elix_cstring_length(string, 1);
	static char allowed[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-.[]";
	while( length && string[0] == '.' ) {
		// Don't start with a .
		memmove(string, string + 1, length-1);
		length--;
	}
	while( (pos = elix_cstring_find_not_of(string, allowed, pos)) != SIZE_MAX ){
		memmove(string + pos, string + pos + 1, length - pos - 1);
	}
}

size_t elix_cstring_trim( char * string ) {
	size_t length = elix_cstring_length(string, 0);
	//Remove Whitespace from the front
	while( length && (string[0] == ' ' || string[0] == '\r' || string[0] == '\n'|| string[0] == '\t')) {
		memmove(string, string + 1, length-1);
		length--;
	}

	// Checks last character, set it \0 if whitespace and repeat
	if ( length ) {
		for (size_t c = length-1; c >= 0 && (string[c] == ' ' || string[c] == '\r' || string[c] == '\n' || string[c] == '\t'); c--) {
			string[c] = 0;
			length--;
		}
	}
	return length;
}


char * elix_cstring_copy( const char * source, size_t max_length ) {
	ASSERT( source != nullptr )

	size_t length = (max_length == SIZE_MAX) ? elix_cstring_length(source, 1) : max_length;

	if ( length <= 1 ) {
		LOG_ERROR("elix_cstring_copy failed, source was empty");
		LOG_ERROR("Source: %s " pZD, source, max_length);
		return nullptr;
	}

	char * dest = new char[length]();
	memcpy(dest, source, length-1);

	return dest;
}

char * elix_cstring_from( const char * source, const char * default_str, size_t max_length ) {
	ASSERT( default_str != nullptr )

	const char * ptr = source != nullptr ? source : default_str;
	size_t length = (max_length == SIZE_MAX) ? elix_cstring_length(ptr, 1) : max_length;

	if ( length <= 1 ) {
		LOG_ERROR("elix_cstring_from failed, source was empty");
		LOG_ERROR("Source: %s %s " pZD,source, default_str, max_length);
		return nullptr;
	}

	char * dest = new char[length]();
	memcpy(dest, ptr, length-1);

	return dest;
}

uint32_t elix_cstring_next_character(char *& object) {
	uint8_t single = *object;
	uint32_t cchar = single;
	if ( cchar <= 128 )	{

	} else if ( cchar < 224 ) {
		object++;
		uint32_t next = *object;

		cchar = ((cchar << 6) & 0x7ff) + (next & 0x3f);
	} else if ( cchar < 240 )	{
		uint32_t next;

		object++;
		next = (*object) & 0xff;
		cchar = ((cchar << 12) & 0xffff) + ((next << 6) & 0xfff);

		object++;
		next = (*object) & 0x3f;
		cchar += next;
	}	else if ( cchar < 245 )	{
		uint32_t next;

		object++;
		next = (*object) & 0xff;
		cchar = ((cchar << 18) & 0xffff) + ((next << 12) & 0x3ffff);

		object++;
		next = (*object) & 0xff;
		cchar += (next << 6) & 0xfff;

		object++;
		next = (*object) & 0x3f;
		cchar += next;
	}
	object++;
	return cchar;
}

uint32_t elix_cstring_peek_character(char * object) {
	return elix_cstring_next_character(object);
}
