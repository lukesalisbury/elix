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

#ifndef ELIX_STRING_HEADER
#define ELIX_STRING_HEADER

#include "elix_core.h"

/* 
elix_string - Stores NUL terminated string. 
elix_string_buffer- a elix_string with iter
elix_string_pointer - pointer to range of text in a elix_string_buffer
*/

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


#ifdef __cplusplus
extern "C" {
#endif








#ifdef __cplusplus
}
#endif









#endif // ELIX_STRING_H
