/***********************************************************************************************************************
Copyright © Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If
   you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not
   required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
   software.
3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/

#ifndef ELIX_JSON_HEADER
#define ELIX_JSON_HEADER

#include "elix_core.h"
#include "elix_parse.h"
#include "elix_memory.h"

#define EJ_IGNORE_ARRAY 0x01
#define EJ_ALOT_TOKEN 0x8000

typedef enum elix_json_type {
	JSON_ERROR,
	JSON_PRIMITIVE,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_UNKNOWN
} elix_json_type;


#define ELIX_JSON_OBJECT_POOL_SIZE 64

typedef struct elix_json_object {
	uint32_t keys[ELIX_JSON_OBJECT_POOL_SIZE];
	elix_string_pointer values[ELIX_JSON_OBJECT_POOL_SIZE];
	uint8_t used;
} elix_json_object;


typedef struct elix_json_token {
	uint16_t type;
	uint16_t parent;
	elix_string_index key;
	elix_string_index data;
} elix_json_token;

typedef struct elix_json {
	elix_string_buffer * reference;
	elix_json_token * tokens;
	uint16_t token_limit;
	uint16_t index;
	uint16_t options;
} elix_json;

#ifdef __cplusplus
extern "C" {
#endif

elix_json elix_json_open(elix_string_buffer * content, uint16_t options);
void elix_json_close(elix_json * doc);


//elix_json_object elix_json_get_object( elix_json * json, elix_json_token token);

void elix_json_print(elix_json * doc);


#ifdef __cplusplus
}
#endif


#endif // ELIX_JSON_HEADER
