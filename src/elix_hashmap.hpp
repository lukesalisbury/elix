#ifndef ELIX_HASHMAP_HPP
#define ELIX_HASHMAP_HPP

#include "elix_core.h"

#define ELIX_HASHMAP_POOL_SIZE 32

struct elix_hashmap_pool {
	uint8_t used = 0;
	uint32_t keys[ELIX_HASHMAP_POOL_SIZE];
	data_pointer values[ELIX_HASHMAP_POOL_SIZE];
};

struct elix_hashmap {
	elix_hashmap_pool active;
	elix_hashmap * next = nullptr;
};

elix_hashmap * elix_hashmap_create();
bool elix_hashmap_destroy(elix_hashmap ** hm, void (*delete_func)(data_pointer*) = nullptr);

void elix_hashmap_insert_hash(elix_hashmap * hm, uint32_t key, data_pointer value);
data_pointer elix_hashmap_value_hash(elix_hashmap * hm, uint32_t key);
void elix_hashmap_remove_hash(elix_hashmap * hm, uint32_t key, void (*delete_func)(data_pointer*) = nullptr);

void elix_hashmap_insert(elix_hashmap * hm, const char * key, data_pointer value);
void elix_hashmap_remove(elix_hashmap * hm, const char * key, void (*delete_func)(data_pointer*) = nullptr);
data_pointer elix_hashmap_value(elix_hashmap * hm, const char * key);

#endif // ELIX_HASHMAP_HPP
