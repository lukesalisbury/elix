#include "elix_parse.h"

elix_string elix_string_new(uint16_t allocate) {
	elix_string str = {0};

	if ( allocate ) {
		str.data = ALLOCATE(uint8_t, allocate);
		str.allocated = allocate;
		str.owned_data = 1;
	}
	return str;
}

void elix_string_clear(elix_string * str) {
	if ( str->owned_data ) {
		memset(str->data, 0, str->allocated);
		str->length = 0;
	}
}

void elix_string_append_data(elix_string * str, uint8_t * data, uint16_t data_size ) {
	///TODO: Only Resize when data is owned
	//resize string
	if ( str->length + data_size >= str->allocated - 1 ) {
		void * newptr = realloc(str->data, str->length + data_size + 8 );
		if ( !newptr ) {
			LOG_ERROR("String couldn't be resized");
			return;
		}
		str->data = (uint8_t*)newptr;
		str->allocated = str->length + data_size + 8;
	}

	if ( str->location >= str->allocated - 1 ) {
		ASSERT("Invaild String location");
	}

	memcpy(str->data + str->length, data, data_size);
	str->length += data_size;
	str->location += data_size;

}
void elix_string_append_byte(elix_string * str, uint8_t byte ) {
	//resize string
	if ( str->length >= str->allocated - 1 ) {
		void * newptr = realloc(str->data, str->allocated + 8 );
		if ( !newptr ) {
			LOG_ERROR("String couldn't be resized");
			return;
		}
		str->data = (uint8_t*)newptr;
		str->allocated += 8;
	}

	str->data[str->length] = byte;
	str->length++;
}

void elix_string_append(elix_string * str, uint32_t char32 ) {

	uint8_t char8 = 0;
	if ( char32 <= 0x7F ) {
		char8 = char32 & 0x7F;
		elix_string_append_byte(str, char8);
	} else if (char32 <= 0x07FF) {
		char8 = ( (char32 >> 6) & 0x1F) | 0xC0;
		elix_string_append_byte(str, char8);
		char8 = (char32 & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
	} else if (char32 <= 0xFFFF) {
		char8 = ( (char32 >> 12) & 0x0F) | 0xE0;
		elix_string_append_byte(str, char8);
		char8 = ( (char32 >> 6) & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
		char8 = (char32 & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
	} else if (char32 <= 0x10FFFF) {
		char8 = ( (char32 >> 18) & 0x07) | 0xF0;
		elix_string_append_byte(str, char8);
		char8 = ( (char32 >> 12) & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
		char8 = ( (char32 >> 6) & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
		char8 = (char32 & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
	} else {
		elix_string_append_byte(str, 0xEF);
		elix_string_append_byte(str, 0xBF);
		elix_string_append_byte(str, 0xBD);
	}
}



elix_string_buffer elix_string_buffer_new(const char * string, size_t buffer_size) {
	elix_string_buffer str = {0};
	if ( buffer_size >= __UINT16_MAX__ || string == nullptr || buffer_size == 0) {
		///TODO: Handle long text
		return str;
	}

	str.string.allocated = buffer_size + 1;
	str.string.data = ALLOCATE(uint8_t, str.string.allocated);
	str.string.owned_data = 1;
	if (string) {
		const char *counter = string;
		while(*counter++) {
			++ str.string.length;
		}
		str.string.length = str.string.length < buffer_size ? str.string.length : buffer_size;
		memcpy(str.string.data, string, str.string.length);
		
	}

	str.string.location = 0;
	str.iter = str.string.data;

	return str;
}

void elix_string_buffer_delete( elix_string_buffer * buffer) {
	if (buffer->string.owned_data) {
		NULLIFY(buffer->string.data);
		buffer->string.allocated = 0;
	} 
	buffer->string.location = 0;
	buffer->string.length = 0;
}

size_t elix_string_buffer_move( elix_string_buffer * str, size_t position ) {
	if ( position >= str->string.length ) {
		str->string.location = 0;
		str->iter = str->string.data;
		return 0;
	}
	str->string.location = position;
	str->iter = str->string.data + str->string.location;
	return position;
}

uint32_t elix_string_buffer_forward( elix_string_buffer * str, uint32_t count ) {
	str->string.location += count;
	if ( str->iter ) {
		str->iter += count;
	} else {
		str->iter = str->string.data + str->string.location;
	}
	return count;
}



elix_string_pointer elix_string_buffer_get_pointer( elix_string_buffer * buffer, size_t offset, size_t length ) {
	elix_string_pointer str;

	if ( offset + length >= buffer->string.length ) {
		return str;
	}

	str.source = buffer;
	str.string = buffer->string.data + offset;
	str.offset = offset;
	str.length = length;

	return str;
}

uint32_t elix_string_buffer_peek_match(elix_string_buffer * text, size_t index, const char * needle, uint8_t needle_length) {
	ASSERT(index < text->string.length);
	uint8_t * iter = text->string.data + index;
	for (uint8_t i = 0; i < needle_length; i++) {
		if ( iter[i] != needle[i] ) {
			return 0;
		}
	}
	return needle_length;
}

/* Get a printable single utf8 character. */
static char elix_utf8_string[6] = {0};

char * elix_character_print(elix_character char32) {
	memset(elix_utf8_string, 0, 6);
	if (char32.value <= 0x7F) {
		elix_utf8_string[0] = (char)char32.value;
	} else if (char32.value <= 0x7FF) {
		elix_utf8_string[0] = (char)(0xC0 | ((char32.value >> 6) & 0x1F));
		elix_utf8_string[1] = (char)(0x80 | (char32.value & 0x3F));
	} else if (char32.value <= 0xFFFF) {
		// 3-byte sequence
		elix_utf8_string[0] = (char)(0xE0 | ((char32.value >> 12) & 0x0F));
		elix_utf8_string[1] = (char)(0x80 | ((char32.value >> 6) & 0x3F));
		elix_utf8_string[2] = (char)(0x80 | (char32.value & 0x3F));
	} else if (char32.value <= 0x10FFFF) {
		// 4-byte sequence
		elix_utf8_string[0] = (char)(0xF0 | ((char32.value >> 18) & 0x07));
		elix_utf8_string[1] = (char)(0x80 | ((char32.value >> 12) & 0x3F));
		elix_utf8_string[2] = (char)(0x80 | ((char32.value >> 6) & 0x3F));
		elix_utf8_string[3] = (char)(0x80 | (char32.value & 0x3F));
	}
	return elix_utf8_string;
}

elix_character elix_character_next( elix_string_buffer * text ) {
	elix_character char32 = {0,1,0,1};

	uint8_t char8 = 0;
	uint32_t next;

	char8 = (*text->iter);
	if ( char8 < 128 ) {
		char32.value = char8;
		
		char32.codepage = 0; //ASCII
	} else if ( char8 > 193 && char8 < 224 ) {
		elix_string_buffer_forward(text, 1);
		next = (*text->iter);

		char32.value = ((char8 << 6) & 0x7ff) + (next & 0x3f);
		char32.bytes = 2;
	} else if ( char8 > 223 && char8 < 240 ) {
		elix_string_buffer_forward(text, 1);
		next = (*text->iter) & 0xff;
		char32.value = ((char8 << 12) & 0xffff) + ((next << 6) & 0xfff);

		elix_string_buffer_forward(text, 1);
		next = (*text->iter) & 0x3f;
		char32.value += next;
		char32.bytes = 3;
	} else if ( char8 > 239 && char8 < 245 ) {
		elix_string_buffer_forward(text, 1);
		next = (*text->iter) & 0xff;
		char32.value = ((char8 << 18) & 0xffff) + ((next << 12) & 0x3ffff);

		elix_string_buffer_forward(text, 1);
		next = (*text->iter) & 0xff;
		char32.value += (next << 6) & 0xfff;

		elix_string_buffer_forward(text, 1);
		next = (*text->iter) & 0x3f;
		char32.value += next;
		char32.bytes = 4;
	} else {
		char32.value = 0xFFFD;
		char32.bytes = 2; ///TODO: Might be wrong
	}
	elix_string_buffer_forward(text, 1);
	return char32;
}


bool elix_parse_isOpenTagChar(uint32_t c) {
	return ( c == '<');
}
bool elix_parse_isEndTagChar(uint32_t c) {
	return ( c == '>');
}
bool elix_parse_isVoidTagChar(uint32_t c) {
	return ( c == '/');
}

bool elix_parse_isWhiteSpace(uint32_t c) {
	return ( c == ' ' || c == '\t' || c == '\n' );
}

bool elix_parse_isValidNameChar(uint32_t c, uint8_t first) {
	if ( ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
		return true;
	} else if ( !first && ((c >= '0' && c <= '9') || (c == '-'))) {
		return true;
	} else {
		return false;
	}
}


uint32_t elix_parse_lowerCaseASCII(uint32_t c) {
	if ( (c >= 'A' && c <= 'Z') ) {
		return c + 32;
	} else {
		return c;
	}
}

bool elix_parse_isPrimitiveChar(uint32_t c, uint32_t counter) {
	if ( counter == 0 && (c == 't' || c == 'f' || c == 'n' ) ) {
		return true;
	} else {
		return false;
	}
}

bool elix_parse_isPrimitiveNumber(uint32_t c, uint32_t counter) {
	///TODO: Handle exponent
	///TODO: Limit only one .
	if ( !counter && c == '-' ) {
		return true;
	} else if ( counter && c == '.' ) {
		return true;
	} else if ( c >= '0' && c <= '9' ) {
		return true;
	} else {
		return false;
	}
}


