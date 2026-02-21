#ifndef ELIX_PARSE_HEADER
#define ELIX_PARSE_HEADER

#include "elix_core.h"
#include <stdlib.h>
#include <string.h>

typedef struct elix_string {
	uint8_t * text;
	bool16 owned;
	uint16_t length;
	uint16_t allocated;
	uint16_t location;
} elix_string;


typedef struct elix_string_buffer {
	uint8_t * data;
	uint8_t * iter;
	bool16 owned;
	uint32_t length;
	uint32_t allocated;
	uint32_t location;
} elix_string_buffer;

typedef struct elix_string_pointer {
	elix_string_buffer * source;
	uint8_t * string;
	size_t offset;
	size_t length;
} elix_string_pointer;

typedef struct elix_character {
	uint32_t value;
	uint8_t bytes;
	uint8_t padding;
	uint16_t codepage; //0 = ASCII, elsewise Unicode
} elix_character;


typedef struct elix_parse_status {
	size_t offset;
	size_t length;
} elix_parse_status;

#ifdef __cplusplus
extern "C" {
#endif

elix_string elix_string_new(uint16_t allocate);
void elix_string_clear(elix_string * str);
void elix_string_append_byte(elix_string * str, uint8_t byte );
void elix_string_append(elix_string * str, uint32_t char32 );

uint32_t elix_string_buffer_forward( elix_string_buffer * str, uint32_t count );
elix_string_pointer elix_string_buffer_get_pointer( elix_string_buffer * buffer, size_t offset, size_t length );
elix_character elix_string_buffer_next( elix_string_buffer * text );
elix_character elix_character_next( elix_string_buffer * text );



bool isOpenTagChar(uint32_t c);
bool isEndTagChar(uint32_t c);
bool isVoidTagChar(uint32_t c);
bool isWhiteSpace(uint32_t c);
bool isValidNameChar(uint32_t c, uint8_t first);

uint32_t lowerCaseASCII(uint32_t c);

#ifdef __cplusplus
}
#endif
#endif // ELIX_PARSE_HEADER