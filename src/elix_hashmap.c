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

#include "elix_hashmap.h"
#include "elix_cstring.h"
#include <string.h>

elix_hashmap * elix_hashmap_create() {
	elix_hashmap * hm = ALLOCATE(elix_hashmap, 1);
	return hm;
}

void elix_hashmap_insert_hash(elix_hashmap * hm, uint32_t key, data_pointer value) {
	if ( hm->active.used < ELIX_HASHMAP_POOL_SIZE ) {
		uint32_t * current = &hm->active.keys[0];
		for (size_t c = 0; c< ELIX_HASHMAP_POOL_SIZE; c++, current++ ) {
			if ( *current == 0) {
				if ( hm->active.values[c] == nullptr ) {
					*current = key;
					hm->active.values[c] = value;
					hm->active.used++;
					return;
				} else {
					LOG_MESSAGE("Hashmap has no nullptr value");
				}
			}
		}
	}
	if ( !hm->next ) {
		hm->next = ALLOCATE(elix_hashmap, 1);
	}
	if ( !hm->next ) {
		LOG_MESSAGE("Can not create hashmap");
		return;
	}
	elix_hashmap_insert_hash(hm->next, key, value);
}

data_pointer elix_hashmap_value_hash(elix_hashmap * hm, uint32_t key) {
	if ( !hm || !hm->active.used) {
		return nullptr;
	}

	uint32_t * current = &hm->active.keys[0];
	for (size_t c = 0; c< ELIX_HASHMAP_POOL_SIZE; c++, current++ ) {
		if ( *current == key) {
			if ( hm->active.values[c] == nullptr ) {
				LOG_MESSAGE(pZU " is empty", c);
			}
			return hm->active.values[c];
		}
	}
	if ( hm->next ) {
		return elix_hashmap_value_hash(hm->next, key);
	}

	return nullptr;
}

void elix_hashmap_remove_hash(elix_hashmap * hm, uint32_t key, void (*delete_func)(data_pointer*)) {
	if ( !hm->active.used && !hm->next ) {
		return;
	}

	uint32_t * current = &hm->active.keys[0];
	for (size_t c = 0; c< ELIX_HASHMAP_POOL_SIZE; c++, current++ ) {
		if ( *current == key) {
			*current = 0;
			if ( delete_func ) {
				delete_func(&hm->active.values[c]);
			} else {
				hm->active.values[c] = nullptr;
			}
			return;
		}
	}
	if ( hm->next ) {
		return elix_hashmap_remove_hash(hm->next, key, delete_func);
	}
	return;
}

void  elix_hashmap_clear(elix_hashmap * hm, void (*delete_func)(data_pointer*)) {
	if ( hm->next ) {
		elix_hashmap_clear(hm->next,delete_func);
		NULLIFY(hm->next);
	}
	if ( delete_func ) {
		data_pointer * current = &hm->active.values[0];
		for (size_t c = 0; c< ELIX_HASHMAP_POOL_SIZE; c++, current++ ) {
			delete_func(current);
		}
	}
	hm->active.used = 0;
	memset(&hm->active.keys, 0 , sizeof(uint32_t)*ELIX_HASHMAP_POOL_SIZE);
	memset(&hm->active.values, 0 , sizeof(uintptr_t)*ELIX_HASHMAP_POOL_SIZE);
}

bool elix_hashmap_destroy(elix_hashmap ** hm, void (*delete_func)(data_pointer*)) {
	elix_hashmap_clear((*hm), delete_func);
	NULLIFY(*hm);
	return false;
}

void elix_hashmap_insert(elix_hashmap * hm, const char * key, data_pointer value) {
	uint32_t hash = elix_hash(key, elix_cstring_length(key, 0));
	elix_hashmap_insert_hash( hm, hash, value);
}

void elix_hashmap_remove(elix_hashmap * hm, const char * key, void (*delete_func)(data_pointer*)) {
	uint32_t hash = elix_hash(key, elix_cstring_length(key, 0));
	return elix_hashmap_remove_hash(hm, hash, delete_func);
}

data_pointer elix_hashmap_value(elix_hashmap * hm, const char * key) {
	uint32_t hash = elix_hash(key, elix_cstring_length(key, 0));
	return elix_hashmap_value_hash(hm, hash);
}

