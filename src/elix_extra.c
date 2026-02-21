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
#include "elix_extra.h"


void elix_ucstring_copy( const unsigned char * source_init, char * dest_init, size_t dest_size) {
	unsigned char * source = (unsigned char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
		dest_size--;
	} while( dest_size && *source != 0);
	*dest = '\0';
}

elix_http_request elix_http_request_parse(elix_allocated_buffer * buffer, bool header_copy, bool body_copy) {
	elix_http_request request = {0};
	size_t header_position = 0, header_line_end = 0;
	size_t header_size = elix_cstring_find_of((char *)buffer->data, "\r\n\r\n", 3);
	char * headers[32] = {nullptr};
	uint8_t headers_count = 0;
	char * text_buffer = (char *)buffer->data;
	
	//if ( header_copy ) {
	//	request.headers_buffer = (char *)calloc(header_size, 1);
	//	elix_cstring_copy((char *)buffer.data, request.headers_buffer, header_size);
	//	request.headers_buffer_size = header_size;
	//	text_buffer = (char *)request.headers_buffer;
	//}
	//if ( body_copy ) {
	//	request.body_buffer = (char *)calloc(buffer.actual_size - header_size, 1);
	//	elix_cstring_copy((char *)buffer.data + header_size, request.body_buffer, buffer.actual_size - header_size);
	//	request.body_buffer_size = buffer.actual_size - header_size;
	//}

	if ( header_size == SIZE_MAX ) {
		LOG_ERROR("No headers found");
		return request;
	}

	while ( header_position < header_size ) {
		header_line_end = elix_cstring_find_of(text_buffer, "\r\n", 2);

		if ( header_line_end < header_size) {
			text_buffer[header_line_end] = 0;
			if ( headers_count < 32 ) {
				headers[headers_count++] = text_buffer;
				if ( elix_cstring_has_prefix(text_buffer, "Host: ") ) {
					request.host = text_buffer + 6;
				} else if ( elix_cstring_has_prefix(text_buffer, "Accept: ") ) {
					request.accept = text_buffer + 8;
				} else if ( elix_cstring_has_prefix(text_buffer, "Connection: ") ) {
					request.connection = text_buffer + 12;
				} else if ( elix_cstring_has_prefix(text_buffer, "Cookie: ") ) {
					request.cookie = text_buffer + 8;
				}
			}
			header_line_end += 2; //Skip \r\n
			text_buffer += header_line_end;
			header_size -= header_line_end;
		}
		header_position = header_line_end;
	}

	size_t firstline_split = elix_cstring_find_of(headers[0], " ", 0);
	if ( firstline_split == SIZE_MAX ) {
		LOG_ERROR("No method found");
		return request;
	}

	request.method = headers[0];
	request.method[firstline_split] = 0;

	firstline_split++;
	request.uri = headers[0] + firstline_split;

	firstline_split = elix_cstring_find_of(request.uri, " ", 0);
	if ( firstline_split == SIZE_MAX ) {
		LOG_ERROR("No uri found");
		return request;
	}
	request.uri[firstline_split] = 0;

	return request;
}

int64_t elix_cstring_convert_to_int(const char * str) {
	if ( str == nullptr ) {
		return 0;
	}

	int64_t result = 0;
	uint8_t c, v;
	uint8_t hex = ( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') );
	if ( hex ) {
		str += 2;
		while ((c = *str++)) {
			if ( ('0' <= c && c <= '9') || ( 'a' <= c && c <= 'f') || ( 'A' <= c && c <= 'F') ) {
				v = (c & 0xF) + (c >> 6) | ((c >> 3) & 0x8);
				result = (result << 4) | (uint64_t) v;
			}
			
		}
	} else {
		while ( (c = *str++)) {
			if ( ('0' <= c && c <= '9') ) {
				result = result * 10 + (c - '0');
			}
			
		}
	}
	
	return result;
}
