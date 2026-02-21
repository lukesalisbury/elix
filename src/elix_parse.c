#include "elix_parse.h"

elix_string elix_string_new(uint16_t allocate) {
    elix_string str = {};

    if ( allocate ) {
        str.text = ALLOCATE(uint8_t, allocate);
        str.allocated = allocate;
		str.owned = 1;
    }
    return str;
}

void elix_string_clear(elix_string * str) {
	if ( str->owned ) {
		memset(str->text, 0, str->allocated);
		str->length = 0;
	}
}

void elix_string_append_byte(elix_string * str, uint8_t byte ) {
	//resize string
	if ( str->length >= str->allocated - 1 ) {
		void * newptr = realloc(str->text, str->allocated + 8 );
		if ( !newptr ) {
			LOG_ERROR("String couldn't be resized");
			return;
		}
		str->text = (uint8_t*)newptr;
		str->allocated += 8;
	}

	str->text[str->length] = byte;
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

uint32_t elix_string_buffer_forward( elix_string_buffer * str, uint32_t count ) {
	str->location += count;
	if ( str->iter ) {
		str->iter += count;
	} else {
		str->iter = str->data + str->location;
	}
	return count;
}



elix_string_pointer elix_string_buffer_get_pointer( elix_string_buffer * buffer, size_t offset, size_t length ) {
    elix_string_pointer str;

	if ( offset + length >= buffer->length ) {
		return str;
	}

	str.source = buffer;
	str.string = buffer->data + offset;
	str.offset = offset;
	str.length = length;

    return str;
}

elix_character elix_string_buffer_next( elix_string_buffer * text ) {
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


bool isOpenTagChar(uint32_t c) {
	return ( c == '<');
}
bool isEndTagChar(uint32_t c) {
	return ( c == '>');
}
bool isVoidTagChar(uint32_t c) {
	return ( c == '/');
}

bool isWhiteSpace(uint32_t c) {
    return ( c == ' ' || c == '\t' || c == '\n' );
}

bool isValidNameChar(uint32_t c, uint8_t first) {
	if ( ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
		return true;
	} else if ( !first && ((c >= '0' && c <= '9') || (c == '-'))) {
		return true;
	} else {
		return false;
	}
}


uint32_t lowerCaseASCII(uint32_t c) {
	if ( (c >= 'A' && c <= 'Z') ) {
		return c + 32;
	} else {
		return c;
	}
}
