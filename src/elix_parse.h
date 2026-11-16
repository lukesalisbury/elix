#ifndef ELIX_PARSE_HEADER
#define ELIX_PARSE_HEADER

#include "elix_core.h"
#include <stdlib.h>
#include <string.h>

typedef struct elix_string {
	uint8_t * data;
	uint32_t length;
	uint32_t allocated;
	uint32_t location;
	uint32_t owned_data;
} elix_string;

typedef struct elix_string_buffer {
	elix_string string;
	uint8_t * iter;
} elix_string_buffer;

typedef struct elix_string_index {
	size_t offset;
	size_t length;
} elix_string_index;

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


typedef uint16_t elix_parse_listsize;

#ifdef __cplusplus
extern "C" {
#endif

elix_string elix_string_new(uint16_t allocate);
void elix_string_clear(elix_string * str);
void elix_string_append_data(elix_string * str, uint8_t * data, uint16_t data_size );
void elix_string_append_byte(elix_string * str, uint8_t byte );
void elix_string_append(elix_string * str, uint32_t char32 );



elix_string_buffer elix_string_buffer_new( const char * string, size_t length );
void elix_string_buffer_delete( elix_string_buffer * buffer );

size_t elix_string_buffer_move( elix_string_buffer * str, size_t position );
uint32_t elix_string_buffer_forward( elix_string_buffer * str, uint32_t count );
elix_string_pointer elix_string_buffer_get_pointer( elix_string_buffer * buffer, size_t offset, size_t length );
elix_character elix_string_buffer_next( elix_string_buffer * text );
uint32_t elix_string_buffer_peek_match( elix_string_buffer * text, size_t index, const char * needle, uint8_t needle_length );

elix_character elix_character_next( elix_string_buffer * text );

bool elix_parse_isOpenTagChar(uint32_t c);
bool elix_parse_isEndTagChar(uint32_t c);
bool elix_parse_isVoidTagChar(uint32_t c);
bool elix_parse_isWhiteSpace(uint32_t c);
bool elix_parse_isValidNameChar(uint32_t c, uint8_t first);
bool elix_parse_isPrimitiveChar(uint32_t c, uint32_t counter);
bool elix_parse_isPrimitiveNumber(uint32_t c, uint32_t counter);

uint32_t elix_parse_lowerCaseASCII(uint32_t c);




#ifdef __cplusplus
}
#endif
#endif // ELIX_PARSE_HEADER
