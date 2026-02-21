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

#include "elix_cstring.h"
#include <string.h>
#include <stdlib.h>

size_t elix_ucstring_length(const uint8_t * string, uint8_t include_terminator ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}
size_t elix_cstring_length(const char * string, uint8_t include_terminator ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}

void elix_cstring_clear( char * string, const size_t len ) {
	if (string) {
		size_t c = 0;
		do {
			*string = 0;
			string++;
			c++;
		} while( *string != 0 && c < len);
	}
}

char * elix_cstring_substr( const char * source, ssize_t pos, ssize_t len ) {
	ASSERT(source != nullptr)

	size_t offset = 0;
	size_t max_length = elix_cstring_length(source, 0);
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

	char * dest = ALLOCATE(char, length + 1);
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

uint8_t elix_cstring_compare( const char * primary, const char * secondary) {
	size_t primary_length = elix_cstring_length(primary, 0);
	size_t secondary_length = elix_cstring_length(secondary, 0);

	if ( primary_length != secondary_length )
		return false;

	//TODO: Add case check
	for (size_t c = 0; c < primary_length; c++ ) {
		if ( primary[c] != secondary[c] )
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
	str[length] = 0;
	return length;
}


size_t elix_cstring_find_not_of( const char * str, const char * search, size_t offset) {
	size_t str_len = elix_cstring_length(str,0);
	size_t srh_len = elix_cstring_length(search,0);
	for (size_t str_c = offset; str_c < str_len && str[str_c] != 0; str_c++) {
		uint8_t found = false;
		for (size_t srh_c = 0; srh_c < srh_len; srh_c++) {
			if ( str[str_c] == search[srh_c]) {
				found = 1;
				break;
			}
		}
		if ( !found )
			return str_c;
	}
	return SIZE_MAX;
}

size_t elix_cstring_find_of(const char * str, const char * search, size_t offset) {
	size_t length = elix_cstring_length(str,0);
	size_t sl = elix_cstring_length(search,0);
	for (size_t c = offset; c < length - (sl-1) && str[c] != 0; c++) {
		bool found = false;
		for (size_t sc = 0; sc < sl; sc++) {
			if ( str[c+sc] == search[sc]) {
				found = (sc == sl-1);
			} else {
				break;
			}
		}
		if ( found )
			return c;
	}
	return SIZE_MAX;
}

size_t elix_cstring_find_last_of(const char * str, const char * search, size_t offset) {
	size_t length = elix_cstring_length(str, 0);
	size_t sl = elix_cstring_length(search, 0);
	for (size_t c = length - sl; c > offset; c--) {
		bool found = false;
		for (size_t sc = 0; sc < sl; sc++) {
			if ( str[c+sc] == search[sc]) {
				found = (sc == sl-1);
			} else {
				break;
			}
		}
		if ( found ) {
			return c;
		}
	}
	return SIZE_MAX;
}

size_t elix_cstring_inreplace( char * source_text, size_t buffer_size, const char *search, const char *replace) {
	//NOTE: This is slow, and not a good implemenation
	//TODO: Check for overflow
	size_t search_len = 0, replace_len = 0;
	ssize_t diff_len = 0;
	size_t source_len = elix_cstring_length(source_text, 1);
	size_t pos = elix_cstring_find_of(source_text, search, 0);
	if ( pos != SIZE_MAX ) {
		search_len = elix_cstring_length(search, 0);
		replace_len = elix_cstring_length(replace, 0);
		diff_len = search_len - replace_len;

		size_t l = 0;
		if ( diff_len > 0 ) {
			for (size_t i = pos; i < source_len; l++,i++) {
				if ( l < replace_len ) {
					source_text[i] = replace[l];
				} else {
					source_text[i] = source_text[i+diff_len];
				}
			}
		} else {
			if ( pos+search_len >= buffer_size) {
				return 0;
			}
			if ( diff_len < 0 ) {
				size_t j = source_len-diff_len;
				for (size_t i = source_len; i >= pos+search_len; i--) {
					j = i-diff_len;
					source_text[j] = source_text[i];
				}
			}
			for (size_t i = pos; i < buffer_size && l < replace_len; l++,i++) {
				source_text[i] = replace[l];
			}
		}

	}
	return diff_len;
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

#define ELIX_IS_WHITESPACE(x) (x == ' ' || x == '\n' || x == '\r')
size_t elix_cstring_trim( char * string ) {
	size_t length = elix_cstring_length(string, 0);

	while( length && ELIX_IS_WHITESPACE(string[0])  ) {
		memmove(string, string + 1, length);
		length--;
	}
	size_t pos = length ? length-1 : 0;
	while( pos && ELIX_IS_WHITESPACE(string[pos]) ) {
		string[pos] = 0;
		length--;
		pos--;
	}
	return length;
}

void elix_cstring_copy( const char * source_init, char * dest_init, size_t dest_size) {
	if ( source_init == nullptr || dest_init == nullptr || dest_size == 0 ) {
		return;
	}
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
		dest_size--;
	} while( dest_size && *source != 0);
	*dest = '\0';
}



char * elix_cstring_from( const char * source, const char * default_str, size_t max_length ) {
	ASSERT( default_str != nullptr )
	const char * ptr = source != nullptr ? source : default_str;
	size_t length = (max_length == SIZE_MAX) ? elix_cstring_length(ptr, 1) : max_length;

	if ( length <= 1 ) {
		if ( default_str != nullptr ) {
			ptr = default_str;
			length = elix_cstring_length(ptr, 1);
		} else {
			LOG_ERROR("elix_cstring_from failed, source was empty");
			return nullptr;
		}
	}

	char * dest = ALLOCATE(char, length);
	elix_cstring_copy(ptr, dest, length-1);
	//dest[length] = 0;
	return dest;
}


uint32_t elix_cstring_next_character(char * object) {
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
	char * object_peek = object;
	return elix_cstring_next_character(object_peek);
}



char ** elix_cstring_split( const char * source, char token, char string_bracket) {
	#define token_cache 32
	size_t source_len = elix_cstring_length(source, 0);
	size_t tokens = 0;
	size_t position[token_cache] = {0};
	size_t maxlength = 0, last_token = 0;
	uint8_t outside_string = 1;
	for (size_t i = 0; i < source_len; i++)	{
		if ( source[i] == token ) {
			if ( outside_string ) {
				maxlength = (i - last_token) > maxlength ? (i - last_token) : maxlength;
				last_token = i;
				if (tokens < token_cache ) {
					position[tokens] = last_token + 1;
				}
				tokens++;
			}
		} else if ( source[i] == string_bracket ) {
			outside_string = !outside_string;
		}
	}
	position[tokens] = source_len+1;
	tokens++;

	
	if (tokens > token_cache ) {
		//TODO
		LOG_ERROR("elix_cstring_split: Too many tokens");
		return nullptr;
	} else {
		char ** output = (char**)calloc((tokens+1), sizeof(char *));
		last_token = 0;
		for (size_t i = 0; i < tokens; i++)	{
			output[i] = elix_cstring_from(source + last_token, "", position[i] - last_token);
			//LOG_INFO("%d > %d - '%s'", last_token, position[i], output[i]);
			last_token = position[i];
		}
		output[tokens] = nullptr;
		return output;
	}
	
	#undef token_cache
}

size_t elix_cstring_dequote( char * string ) {
	size_t length = elix_cstring_length(string, 0);

	while( length && string[0] == '"' ) {
		memmove(string, string + 1, length);
		length--;
	}
	size_t pos = length ? length-1 : 0;
	while( pos && string[pos] == '"' ) {
		string[pos] = 0;
		length--;
		pos--;
	}
	return length;
}

size_t elix_cstring_quote( char * string, size_t string_buffer_size ) {
	size_t text_length = elix_cstring_length(string, 0);
	if ( text_length + 2 > string_buffer_size ) {
		return text_length;
	}
	if ( string[0] != '"' ) {
		string[text_length] = '"';
		string[text_length+1] = '\0';
		text_length++;
		memmove(string + 1, string, text_length);
		string[0] = '"';
		text_length++;
	}
	return text_length;
}

void elix_cstring_transform( char * string, uint8_t mode ) {
	size_t length = elix_cstring_length(string, 0);
	for (size_t var = 0; var < length; ++var) {
		switch (mode) {
			case ELIX_CHAR_LOWER:
				if (string[var] >= 'A' && string[var] <= 'Z') {
					string[var] += 32;
				}
				break;
			case ELIX_CHAR_UPPER:
				if (string[var] >= 'a' && string[var] <= 'z') {
					string[var] -= 32;
				}
				break;
			case ELIX_CHAR_CAPITALISE:
				if (string[var] >= 'a' && string[var] <= 'z') {
					string[var] -= 32;
				}
				mode = 0;
				break;
			default:
				break;
		}
	}
}

