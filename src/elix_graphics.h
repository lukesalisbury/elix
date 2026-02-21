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

#ifndef ELIX_GRAPHICS_HEADER
#define ELIX_GRAPHICS_HEADER

#include "elix_core.h"

typedef struct elix_graphic_data {
	union {
		uint8_t * data;
		uint32_t * pixels;
	};
	uint32_t width;
	uint32_t height;
	uint64_t size;
	uint64_t pixel_count;
	uint32_t ref;
	uint8_t bpp;
	uint8_t format;
	uint16_t _unsued;
} elix_graphic_data;


static inline elix_graphic_data * elix_graphic_data_create_full(elix_uv32_2 dimensions, uint8_t bpp, UNUSEDARG uint8_t format) {
	elix_graphic_data * buffer = ALLOCATE(elix_graphic_data, 1);

	buffer->width = dimensions.width;
	buffer->height = dimensions.height;
	buffer->bpp = bpp;
	buffer->pixel_count = buffer->width * buffer->height;
	buffer->size = buffer->pixel_count * buffer->bpp;
	buffer->pixels = ALLOCATE(uint32_t, buffer->pixel_count);
	buffer->format = format;
	buffer->ref = 1;
	return buffer;
}

static inline elix_graphic_data * elix_graphic_data_create(elix_uv32_2 dimensions) {
	return elix_graphic_data_create_full(dimensions, 4, 0);
}

static inline void elix_graphic_data_ref(elix_graphic_data * buffer) {
	buffer->ref++;
}

static inline elix_graphic_data * elix_graphic_data_destroy(elix_graphic_data * buffer) {
	buffer->ref--;
	if ( !buffer->ref ) {
		NULLIFY(buffer->pixels);
		buffer->pixel_count = buffer->size = 0;
		NULLIFY(buffer);
	}
	return buffer;
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif //ELIX_GRAPHICS_HEADER