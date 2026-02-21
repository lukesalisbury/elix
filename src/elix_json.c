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

typedef enum { 
	PARSE_NONE,
	PARSE_GUESS,
	PARSE_OBJECT,
	PARSE_COLON,
	PARSE_COMMA,
	PARSE_VALUE,
	PARSE_ARRAY,
	PARSE_PRIMITIVE,
	PARSE_STRING,
	PARSE_KEY,
	PARSE_ERROR = 0xFF
} elix_json_parse_state;



//#define STATECHANGE(Q, D) Q; text_length = 0;
#define STATECHANGE(Q, D) Q; text_length = 0; printf("\n%*s " #Q "", D, ">");


inline bool isPrimitiveChar(uint32_t c, uint32_t counter) {
	if ( counter == 0 && (c == 't' || c == 'f' || c == 'n' ) ) {
		return true;
	} else {
		return false;
	}
}

inline bool isPrimitiveNumber(uint32_t c, uint32_t counter) {
	//TODO: Handle exponent
	//TODO: Limit only one .
	if ( !counter && c == '-' ) {
		return true;
	} else if ( counter && c == '.' ) {
		return true;
	} else if ( c >= 0 || c <= 9 ) {
		return true;
	} else {
		return false;
	}
}

uint32_t elix_string_peek_match(elix_string_buffer * text, elix_parse_status status, const char * needle, uint8_t needle_length) {
	text->iter = text->data + status.offset + 1;
	for (uint8_t i = 0; i < needle_length; i++) {
		if ( text->iter[i] != needle[i] ) {
			return 0;
		}
	}
	return needle_length;
}


elix_parse_status elix_json_parse_object(elix_json * doc, uint16_t parent_index, elix_parse_status current, uint8_t d) {
	
	size_t text_length = 0;
	uint16_t index = 0;

	elix_json_parse_state state = PARSE_OBJECT;
	elix_character char32, previous_char32, ending_char32;
	size_t current_offset = 0;
	
	do {
		char32 = elix_character_next(doc->reference);
		
		

		if ( (state != PARSE_STRING && state != PARSE_PRIMITIVE) && isWhiteSpace(char32.value) ) {
			current.offset += char32.bytes;
			continue;
		}

		current_offset = current.offset + char32.bytes;

		text_length++;

		/*
		PRINT("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
		state = STATECHANGE(PARSE_ERROR, d);
		return current;
		*/
		switch (state) {
			case PARSE_OBJECT:
				if ( char32.value == '"' ) {
					state = STATECHANGE(PARSE_KEY, d);
				} else if ( char32.value == ',' ) {
					//Continue as PARSE_OBJECT
				} else if ( char32.value == '}' ) {
					return current;
				} else {
					state = STATECHANGE(PARSE_ERROR, d);
					PRINT("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;
				}
				break;
			case PARSE_KEY:
				if ( char32.value == '"' && previous_char32.value != '\\' ) {
					text_length--; //Remove the last "
					index = ++doc->index;
					doc->tokens[index] = (elix_json_token){ JSON_STRING, parent_index, current_offset - text_length, text_length, 0, 0};
					state = STATECHANGE(PARSE_COLON, d);
				} else if ( char32.value  ) {
					//valid name

				} else {
					state = STATECHANGE(PARSE_ERROR, d);
					PRINT("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;
				}
				
				break;
			case PARSE_COLON:
				if ( char32.value == ':' ) {
					state = STATECHANGE(PARSE_VALUE, d);
				} else {
					PRINT("Error missing : at " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					state = STATECHANGE(PARSE_ERROR, d);
					
					return current;
				}
				break;
			case PARSE_VALUE:
				if ( char32.value == '"' ) {
					doc->tokens[index].data_offset = current_offset + 1;
					state = STATECHANGE(PARSE_STRING, d);
				} else if ( char32.value == '{' ) {
					doc->tokens[index].type = JSON_OBJECT;
					doc->tokens[index].data_offset = current_offset;
					current.offset += char32.bytes;
					current = elix_json_parse_object(doc, index, current, d++);
					doc->tokens[index].data_length = current.offset - doc->tokens[index].data_offset;
					state = STATECHANGE(PARSE_OBJECT, d);
					d++;
				} else if ( char32.value == '[' ) {
					doc->tokens[index].type = JSON_ARRAY;
					doc->tokens[index].data_offset = current_offset;

					state = STATECHANGE(PARSE_ARRAY, d);
				} else if ( isPrimitiveChar(char32.value, text_length - 1) ) {
					if ( elix_string_peek_match(doc->reference, current, "true", 4) ) {
						current.offset += elix_string_buffer_forward(doc->reference, 4) - char32.bytes;

						doc->tokens[index].data_offset = current_offset;
						doc->tokens[index].type = JSON_PRIMITIVE;
						doc->tokens[index].data_length = 4;

						state = STATECHANGE(PARSE_OBJECT, d);
					} else if ( elix_string_peek_match(doc->reference, current, "false", 5) ) {
						current.offset += elix_string_buffer_forward(doc->reference, 5) - char32.bytes;

						doc->tokens[index].data_offset = current_offset;
						doc->tokens[index].type = JSON_PRIMITIVE;
						doc->tokens[index].data_length = 5;

						state = STATECHANGE(PARSE_OBJECT, d);
					} else if ( elix_string_peek_match(doc->reference, current, "null", 4) ) {
						current.offset += elix_string_buffer_forward(doc->reference, 4) - char32.bytes;

						doc->tokens[index].data_offset = current_offset;
						doc->tokens[index].type = JSON_PRIMITIVE;
						doc->tokens[index].data_length = 4;

						state = STATECHANGE(PARSE_OBJECT, d);
					} else {
						PRINT("Error PARSE_VALUE at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 8 ? 0 : 8, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
						state = STATECHANGE(PARSE_ERROR, d);
						return current;
					}
				} else if ( isPrimitiveNumber(char32.value, text_length - 1) ) {
					doc->tokens[index].data_offset = current_offset;
					doc->tokens[index].type = JSON_NUMBER;
					state = STATECHANGE(PARSE_PRIMITIVE, d);
				} else {
					state = STATECHANGE(PARSE_ERROR, d);
					PRINT("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;
				}
				break;
			case PARSE_PRIMITIVE:
				if ( char32.value == ',' ) {
					doc->tokens[index].data_length = text_length;
					state = STATECHANGE(PARSE_OBJECT, d);
				} else if ( isWhiteSpace(char32.value) ) {
					doc->tokens[index].data_length = text_length;
					state = STATECHANGE(PARSE_OBJECT, d);
				} else if ( isPrimitiveNumber(char32.value, text_length) ) {
					int q = 1;
				} else {
					state = STATECHANGE(PARSE_ERROR, d);
					PRINT("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;
				}
				break;
			case PARSE_ARRAY:
				if ( char32.value == ']' ) {
					doc->tokens[index].data_length = current_offset - doc->tokens[index].data_offset + char32.bytes;
					state = STATECHANGE(PARSE_OBJECT, d);
				} else if ( char32.value == '[' ) {
					state = STATECHANGE(PARSE_ERROR, d);
					PRINT("2D array not supported yet. char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;

				} else if ( char32.value ) {

				} else {
					state = STATECHANGE(PARSE_ERROR, d);
					PRINT("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;
				}
				break;
			case PARSE_STRING:
				if ( char32.value == '"' && previous_char32.value != '\\' ) {
					doc->tokens[index].data_length = text_length - 1;
					state = STATECHANGE(PARSE_OBJECT, d);
				} else if ( char32.value  ) {
					//valid name

				} else {
					state = STATECHANGE(PARSE_ERROR, d);
					LOG_INFO("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
					return current;
				}
				break;
			case PARSE_ERROR:
				return current; 
				break;
			default:
				state = STATECHANGE(PARSE_ERROR, d);
				LOG_INFO("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
				return current;
				break;
		}
		
		
		previous_char32 = char32;
		current.offset += char32.bytes;
	} while (char32.value && current.offset < current.length );

	return current;
}

elix_parse_status elix_json_parse(elix_json * doc, elix_parse_status * lastStatus) {

	elix_parse_status current = {0};

	current.length = doc->reference->length;
	if ( lastStatus != nullptr ) {
		//TODO: Continue parsing from a point
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

	elix_json_parse_state state = PARSE_NONE;
	elix_character char32, previous_char32, ending_char32;
	
    //Set up pointer to current character
    doc->reference->iter = doc->reference->data + current.offset;

	do {
		char32 = elix_character_next(doc->reference);

		if ( isWhiteSpace(char32.value) ) {
			current.offset += char32.bytes;
			continue;
		}

		if ( char32.value == '{' ) {
			//First Object has not key
			doc->tokens[0] = (elix_json_token) { JSON_OBJECT, 0, 0, 0, current.offset, current.length - current.offset};
			current = elix_json_parse_object(doc, 0, current, 0);
			doc->tokens[0].data_length = current.offset - doc->tokens[0].data_offset;
			return current;
		} else {
			LOG_INFO("Error at char " pZD "\n%.*s\n%*c^", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
		}
		current.offset += char32.bytes;
	} while (char32.value && current.offset < current.length );

	return current;
}

elix_json elix_json_open(elix_string_buffer * content) {
	elix_json doc = {0};

	size_t first_character = 0;

	while ( first_character < content->length && isWhiteSpace(content->data[first_character]) ) {
	    first_character++;
	}

    if ( content->data[first_character] == '{' ) {
		doc.reference = content;
		
		
		elix_json_parse(&doc, nullptr);

		PRINT("=======================================");
		uint8_t d = 0;
		uint16_t parent = 0;
		for (uint16_t i = 0; i < doc.index; i++) {
			switch (doc.tokens[i].type) {
				case JSON_ERROR:
					/* code */
					break;
				case JSON_OBJECT:
					PRINT("%*s'%.*s' = {", d*2, " ", doc.tokens[i].key_length, doc.reference->data + doc.tokens[i].key_offset);
					parent = i;
					d++;
					break;
				case JSON_ARRAY:
					PRINT("%*s'%.*s' = [Array]", d*2, " ", doc.tokens[i].key_length, doc.reference->data + doc.tokens[i].key_offset);
					if ( parent != doc.tokens[i].parent ) {
						d--;
						PRINT("%*s}", d*2, " ");
					}
					break;
				default:
					if ( parent != doc.tokens[i].parent ) {
						d--;
						PRINT("%*s}", d*2, " ");
					}
					PRINT("%*s'%.*s': '%.*s'", d*2, " ", doc.tokens[i].key_length, doc.reference->data + doc.tokens[i].key_offset, doc.tokens[i].data_length, doc.reference->data + doc.tokens[i].data_offset);

					break;
			}
		}
		PRINT("=======================================");

	} else {
		LOG_ERROR("JSON pages must start with a {");
	}
	
	return doc;
}

