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


typedef enum elix_json_type {
	JSON_ERROR,
    JSON_PRIMITIVE,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
	JSON_NUMBER
} elix_json_type;

typedef struct elix_json_token {
	uint16_t type;
	uint16_t parent;
	size_t key_offset;
	size_t key_length;
	size_t data_offset;
	size_t data_length;
} elix_json_token;

typedef struct elix_json {
	elix_string_buffer * reference;
	elix_json_token tokens[256];
	uint16_t index;
} elix_json;

#ifdef __cplusplus
extern "C" {
#endif

elix_json elix_json_open(elix_string_buffer * data);

#ifdef __cplusplus
}
#endif


#endif // ELIX_JSON_HEADER
