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

#include "elix_json.h"
#include "elix_parse.h"
#include "elix_memory.h"

typedef enum { 
	PARSE_NONE,
	PARSE_OBJECT,
	PARSE_ARRAY,
	PARSE_KEY,
	PARSE_COLON,
	PARSE_OBJECT_VALUE,
	PARSE_ARRAY_VALUE,
	PARSE_VALUE,
	PARSE_PRIMITIVE,
	PARSE_STRING,
	PARSE_COMMA,	
	PARSE_ERROR
} elix_json_parse_state;

char * parse_state_name[] = {
	"PARSE_NONE",
	"PARSE_OBJECT",
	"PARSE_ARRAY",
	"PARSE_KEY",
	"PARSE_COLON",
	"PARSE_OBJECT_VALUE",
	"PARSE_ARRAY_VALUE",
	"PARSE_VALUE",
	"PARSE_PRIMITIVE",
	"PARSE_STRING",
	"PARSE_COMMA",	
	"PARSE_ERROR"
};

char * parse_type_name[] = {
	"JSON_ERROR",
	"JSON_PRIMIT",
	"JSON_OBJECT",
	" JSON_ARRAY",
	"JSON_STRING",
	"JSON_NUMBER"
};

#define STATECHANGE(Q, I) Q; text_counter = 0;
//#define STATECHANGE(Q, I) Q; LOG_PRINT("\n[%03d] " #Q "",  I);
//#define STATECHANGE(Q, I) Q; text_length = 0;
//#define STATECHANGE(Q, I) Q; text_length = 0; printf("\n[%03d] " #Q "",  I);



char * elix_character_print(elix_character char32);

elix_parse_status elix_json_parse_function(elix_json * doc, uint16_t parent_index, elix_parse_status current, elix_json_parse_state parent_state, uint8_t parent_is_read_only) {

	elix_json_parse_state parse_state = parent_state;
	elix_character char32 = {0}, previous_char32 = {0};
	size_t current_position = current.offset, text_counter = 0;
	uint16_t index = doc->index;

	uint8_t read_only = ( doc->options & EJ_IGNORE_ARRAY == EJ_IGNORE_ARRAY && (parent_state == PARSE_ARRAY_VALUE || parent_is_read_only));

	do {
		char32 = elix_character_next(doc->reference);

		if ( index >= doc->token_limit) {
			LOG_INFO("Too many Tokens. Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
			current.offset = current.length;
			return current;
		}
		if ( doc->index >= doc->token_limit) {
			LOG_INFO("Too many Tokens. Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
			current.offset = current.length;
			return current;
		}
		if ( doc->reference->string.location >= doc->reference->string.length ) {
			return current;
		}
		if ( (parse_state != PARSE_STRING && parse_state != PARSE_PRIMITIVE) && elix_parse_isWhiteSpace(char32.value) ) {
			current.offset += char32.bytes;
			continue;
		}

		current_position = current.offset; //Current
		current.offset += char32.bytes; //Next


		switch (parse_state) {
			case PARSE_COMMA://Bug: Will add junk entrie
				if ( char32.value == '}' || char32.value == ']' ) {
					return current;
				} else if ( char32.value == ',' ) {
					parse_state = STATECHANGE((PARSE_ARRAY_VALUE == parent_state ? PARSE_ARRAY_VALUE : PARSE_OBJECT), index);
				} else {
					parse_state = STATECHANGE(PARSE_ERROR, index);
				}
				break;
			case PARSE_OBJECT:
				if ( char32.value == '}' || char32.value == ']' ) {
					return current;
				} else if ( char32.value == '"' ) {
					parse_state = STATECHANGE(PARSE_KEY, index);
				}
				break;
			case PARSE_ARRAY:
				parse_state = STATECHANGE(PARSE_ERROR, index);
				break;

			case PARSE_KEY:
				if ( char32.value == '"' && previous_char32.value != '\\' ) {
					//End of Key, create new token
					if ( read_only ) {
						index = doc->token_limit - 1;
					} else {
						index = ++doc->index;
					}
					doc->tokens[index] = (elix_json_token){ JSON_UNKNOWN, parent_index, {current_position-text_counter, text_counter}, {0, 0}};
					parse_state = STATECHANGE(PARSE_COLON, index);
				} else if ( !char32.value  ) {
					parse_state = STATECHANGE(PARSE_ERROR, index);
					return current;
				} else {
					text_counter += char32.bytes;
				}
				break;
			case PARSE_COLON:
				if ( char32.value == ':' ) {
					parse_state = STATECHANGE(PARSE_OBJECT_VALUE, index);
				} else {
					parse_state = STATECHANGE(PARSE_ERROR, index);
				}
				break;
			case PARSE_ARRAY_VALUE:
				if ( char32.value == ']') {
					return current;
				} else {
					//End of Key, create new token
					if ( read_only ) {
						index = doc->token_limit - 1;
					} else {
						index = ++doc->index;
					}
					doc->tokens[index] = (elix_json_token){ JSON_UNKNOWN, parent_index, {0, 0}, {current.offset, 0}};
				}
			case PARSE_OBJECT_VALUE:
			case PARSE_VALUE:
				if ( char32.value == '{' ) {
					doc->tokens[index].type = JSON_OBJECT; doc->tokens[index].data.offset = current.offset;
					current = elix_json_parse_function(doc, index, current, PARSE_OBJECT, read_only);
					doc->tokens[index].data.length = current.offset - doc->tokens[index].data.offset - 1; // current.offset includes '}'
					if ( !read_only ) {
						index = doc->index;
					}

					parse_state = STATECHANGE(PARSE_COMMA, index);
				} else if ( char32.value == '[' ) {
					doc->tokens[index].type = JSON_ARRAY; doc->tokens[index].data.offset = current.offset;
					current = elix_json_parse_function(doc, index, current, PARSE_ARRAY_VALUE, read_only);
					doc->tokens[index].data.length = current.offset - doc->tokens[index].data.offset - 1; // current.offset includes ']'
					if ( !read_only ) {
						index = doc->index;
					}

					parse_state = STATECHANGE(PARSE_COMMA, index);
				} else if ( char32.value == '"' ) {
					doc->tokens[index].type = JSON_STRING; doc->tokens[index].data.offset = current.offset;
					parse_state = STATECHANGE(PARSE_STRING, index);
				} else if ( char32.value == 't' || char32.value == 'f' || char32.value == 'n' ) { //true, false or null
					if ( elix_string_buffer_peek_match(doc->reference, current_position, "true", 4) ) {
						elix_string_buffer_forward(doc->reference, 3);
						doc->tokens[index].type = JSON_PRIMITIVE; doc->tokens[index].data.offset = current_position; doc->tokens[index].data.length = 4;
						current.offset += 3;
						parse_state = STATECHANGE(PARSE_COMMA, index);
					} else if ( elix_string_buffer_peek_match(doc->reference, current_position, "false", 5) ) {
						elix_string_buffer_forward(doc->reference, 4);
						doc->tokens[index].type = JSON_PRIMITIVE; doc->tokens[index].data.offset = current_position; doc->tokens[index].data.length = 5;
						current.offset += 4;
						parse_state = STATECHANGE(PARSE_COMMA, index);
					} else if ( elix_string_buffer_peek_match(doc->reference, current_position, "null", 4) ) {
						elix_string_buffer_forward(doc->reference, 3);
						doc->tokens[index].type = JSON_PRIMITIVE; doc->tokens[index].data.offset = current_position; doc->tokens[index].data.length = 4;
						current.offset += 3;
						parse_state = STATECHANGE(PARSE_COMMA, index);
					} else {
						parse_state = STATECHANGE(PARSE_ERROR, index);
					}
				} else if ( elix_parse_isPrimitiveNumber(char32.value, 0) ) {
					doc->tokens[index].type = JSON_NUMBER; doc->tokens[index].data.offset = current_position;
					parse_state = STATECHANGE(PARSE_PRIMITIVE, index);
					text_counter += char32.bytes;
				} else {
					parse_state = STATECHANGE(PARSE_ERROR, index);
				}
				break;
			case PARSE_PRIMITIVE:
				if ( char32.value == '}' || char32.value == ']' ) {
					doc->tokens[index].data.length = text_counter;
					return current;
				} else if ( char32.value == ',' ) {
					doc->tokens[index].data.length = text_counter;
					parse_state = STATECHANGE((PARSE_OBJECT == parent_state ? PARSE_OBJECT : PARSE_ARRAY_VALUE), index);
				} else if ( elix_parse_isWhiteSpace(char32.value) ) {
					doc->tokens[index].data.length = text_counter;
					parse_state = STATECHANGE(PARSE_COMMA, index);
				} else if ( elix_parse_isPrimitiveNumber(char32.value, text_counter) ) {
					text_counter++;
				} else {
					parse_state = STATECHANGE(PARSE_ERROR, index);
					return current;
				}
				break;
			case PARSE_STRING:
				if ( char32.value == '"' && previous_char32.value != '\\' ) {
					doc->tokens[index].data.length = text_counter;
					parse_state = STATECHANGE(PARSE_COMMA, index);
				} else if ( !char32.value ) {
					parse_state = STATECHANGE(PARSE_ERROR, index);
				} else {
					text_counter += char32.bytes;
				}
				break;
			default:
				//parse_state = STATECHANGE(PARSE_ERROR, index);
				LOG_PRINT("Error at char " pZD " %s\n%.*s\n%*c^", current_position, parse_state_name[parse_state], (current_position < 4 ? 0 : (int)current_position - 4), doc->reference->iter, (current_position < 4 ? (int)current_position - 1 : 3), '-' );
				return current;
				break;
		}

		previous_char32 = char32;
	
	} while (char32.value && current.offset < current.length );

	return current;
}

elix_parse_status elix_json_parse_object(elix_json * doc, uint16_t parent_index, elix_parse_status current) {
	return elix_json_parse_function(doc, parent_index, current, PARSE_OBJECT, 0);
}

elix_parse_status elix_json_parse(elix_json * doc, elix_parse_status * lastStatus) {
	elix_parse_status current = {0};

	current.length = doc->reference->string.length;
	if ( lastStatus != nullptr ) {
		///TODO: Continue parsing from a point
		if ( lastStatus->offset > current.length ) {
			//Pasted the end of file
			return current;
		}
		if ( lastStatus->length > current.length ) {
			//File Changed
		}
	}

	if ( current.length < 5 ) {
		return current;
	}

	elix_character char32, previous_char32, ending_char32;

	//Set up pointer to current character
	doc->reference->iter = doc->reference->string.data + current.offset;

	do {
		char32 = elix_character_next(doc->reference);

		if ( elix_parse_isWhiteSpace(char32.value) ) {
			current.offset += char32.bytes;
			continue;
		}

		if ( char32.value == '{' ) {
			//First Object has no key
			doc->tokens[0] = (elix_json_token) { JSON_OBJECT, 0, {0, 0}, {current.offset, current.length - current.offset}};
			current.offset += char32.bytes;
			current = elix_json_parse_object(doc, 0, current);
			//doc->tokens[0].data.length = current.offset - doc->tokens[0].data.offset;
			return current;
		} else {
			LOG_INFO(" Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
		}
		current.offset += char32.bytes;
	} while (char32.value && current.offset < current.length );

	return current;
}


void elix_json_print(elix_json *doc) {
	LOG_PRINT("=======================================");
	uint8_t d = 0;
	uint16_t parent = 0;
	for (uint16_t i = 0; i <= doc->index; i++) {
		LOG_PREPRINT("%02d [%02d] [%s]: ", i, doc->tokens[i].parent, parse_type_name[doc->tokens[i].type]);
		switch (doc->tokens[i].type) {
			case JSON_ERROR:
				/* code */
				break;
			case JSON_OBJECT:
				if ( doc->tokens[i].key.length )
					LOG_PRINT("%*s'%.*s' = { Source:'%.*s'", d*2, " ", doc->tokens[i].key.length, doc->reference->string.data + doc->tokens[i].key.offset, doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset);
				else
					LOG_PRINT("%*s{ Source:'%.*s'", d*2, " ", doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset);
				break;
			case JSON_ARRAY:
				parent = i;
				if ( doc->tokens[i].key.length )
					LOG_PRINT("%*s'%.*s' = [ %.*s ] (Length: %d)", d*2, " ", doc->tokens[i].key.length, doc->reference->string.data + doc->tokens[i].key.offset, doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset, doc->tokens[i].data.length);
				else
					LOG_PRINT("%*s[ %.*s ]", d*2, " ", doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset);
				
				break;
			default:
				if ( doc->tokens[i].key.length )
					LOG_PRINT("%*s'%.*s': '%.*s' (%d)", d*2, " ", doc->tokens[i].key.length, doc->reference->string.data + doc->tokens[i].key.offset, doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset, doc->tokens[i].data.length);
				else 
					LOG_PRINT("%*s'%.*s' (Length: %d)", d*2, " ", doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset, doc->tokens[i].data.length);

				break;
		}
	}
	LOG_PRINT("=======================================");
}

void elix_json_print_from(elix_json *doc, uint16_t from) {
	LOG_PRINT("=======================================");
	uint8_t d = 0;
	uint16_t parent = 0;
	for (uint16_t i = from; i < doc->index; i++) {
		if ( doc->tokens[i].parent < from && i != from) {
			break;
		}
		LOG_PREPRINT("%02d [%02d] [%s]: ", i, doc->tokens[i].parent, parse_type_name[doc->tokens[i].type]);
		switch (doc->tokens[i].type) {
			case JSON_ERROR:
				/* code */
				break;
			case JSON_OBJECT:
				if ( doc->tokens[i].key.length )
					LOG_PRINT("%*s'%.*s' = { Source:'%.*s' }", d*2, " ", doc->tokens[i].key.length, doc->reference->string.data + doc->tokens[i].key.offset, doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset);
				else
					LOG_PRINT("%*s{ Source:'%.*s' }", d*2, " ", doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset);
				break;
			case JSON_ARRAY:
				parent = i;
				if ( doc->tokens[i].key.length )
					LOG_PRINT("%*s'%.*s' = [ %.*s ] (Length: %d)", d*2, " ", doc->tokens[i].key.length, doc->reference->string.data + doc->tokens[i].key.offset, doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset, doc->tokens[i].data.length);
				else
					LOG_PRINT("%*s[ %.*s ]", d*2, " ", doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset);
				
				break;
			default:
				if ( doc->tokens[i].key.length )
					LOG_PRINT("%*s'%.*s': '%.*s' (%d)", d*2, " ", doc->tokens[i].key.length, doc->reference->string.data + doc->tokens[i].key.offset, doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset, doc->tokens[i].data.length);
				else 
					LOG_PRINT("%*s'%.*s' (Length: %d)", d*2, " ", doc->tokens[i].data.length, doc->reference->string.data + doc->tokens[i].data.offset, doc->tokens[i].data.length);

				break;
		}
	}
	LOG_PRINT("=======================================");
}


elix_json elix_json_open(elix_string_buffer * content, uint16_t options) {
	elix_json doc = {0};

	size_t first_character = 0;

	while ( first_character < content->string.length && elix_parse_isWhiteSpace(content->string.data[first_character]) ) {
		first_character++;
	}

	size_t arena_size = content->string.length * 2;
	doc.token_limit = (arena_size / sizeof(elix_json_token));
	doc.tokens = calloc(doc.token_limit + 1, sizeof(elix_json_token));

	if ( doc.tokens == nullptr ){
		LOG_ERROR("JSON Token Array wasn't created. ");
		return doc;
	}

	if ( content->string.data[first_character] == '{' ) {
		doc.options = options;
		doc.reference = content;

		elix_json_parse(&doc, nullptr);
	} else {
		LOG_ERROR("JSON pages must start with a {");
	}
	
	return doc;
}

void elix_json_close(elix_json * doc) {
	if (doc) {
		NULLIFY(doc->tokens);
		doc->index = 0;
	}
}