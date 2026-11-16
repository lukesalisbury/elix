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

#ifndef ELIX_MEMORY_HEADER
#define ELIX_MEMORY_HEADER

#include "elix_core.h"

typedef struct elix_memory_arena{
	void * buffer;
	size_t offset;
	size_t capacity;
} elix_memory_arena;

elix_memory_arena elix_memoryarena_new( size_t capacity );

void * elix_memoryarena_alloc( elix_memory_arena * arena, size_t size );

function_results elix_memoryarena_delete( elix_memory_arena * arena );
#endif // ELIX_MEMORY_HEADER
