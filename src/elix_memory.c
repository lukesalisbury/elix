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

#include "elix_memory.h"

elix_memory_arena elix_memoryarena_new( size_t capacity ) {
	elix_memory_arena arena = {0,0,capacity};
	arena.buffer = calloc(1, capacity);

	ASSERT(arena.buffer != nullptr);
	if ( arena.buffer == nullptr ) {
		arena.capacity = 0;
	}

	return arena;
}

void * elix_memoryarena_alloc( elix_memory_arena * arena, size_t size ) {
	ASSERT(arena->capacity != 0 );
	if (arena->offset + size > arena->capacity) {
		return NULL;
	}
	void * ptr = arena->buffer + arena->offset;
	arena->offset += size;
	return ptr;
}

function_results elix_memoryarena_delete( elix_memory_arena * arena ) {
	if ( arena ) {
		if ( arena->buffer ) {
			NULLIFY(arena->buffer);
		}
		arena->capacity = 0;
		arena->offset = 0;
		return RESULTS_SUCCESS;
	}


	return RESULTS_ERROR_ARGUMENT;
}
