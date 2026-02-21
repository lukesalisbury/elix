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

#ifndef ELIX_HASHMAP_HPP
#define ELIX_HASHMAP_HPP

#include "elix_core.h"

#define ELIX_HASHMAP_POOL_SIZE 32

typedef struct elix_hashmap_pool {
	uint8_t used;
	uint32_t keys[ELIX_HASHMAP_POOL_SIZE];
	data_pointer values[ELIX_HASHMAP_POOL_SIZE];
} elix_hashmap_pool;

typedef struct elix_hashmap elix_hashmap;

typedef struct elix_hashmap {
	elix_hashmap_pool active;
	elix_hashmap * next;
} elix_hashmap;

#ifdef __cplusplus
extern "C" {
#endif

elix_hashmap * elix_hashmap_create();
bool elix_hashmap_destroy(elix_hashmap ** hm, void (*delete_func)(data_pointer*));

void elix_hashmap_insert_hash(elix_hashmap * hm, uint32_t key, data_pointer value);
data_pointer elix_hashmap_value_hash(elix_hashmap * hm, uint32_t key);
void elix_hashmap_remove_hash(elix_hashmap * hm, uint32_t key, void (*delete_func)(data_pointer*));

void elix_hashmap_insert(elix_hashmap * hm, const char * key, data_pointer value);
void elix_hashmap_remove(elix_hashmap * hm, const char * key, void (*delete_func)(data_pointer*));
data_pointer elix_hashmap_value(elix_hashmap * hm, const char * key);

#ifdef __cplusplus
}
#endif

#endif // ELIX_HASHMAP_HPP
