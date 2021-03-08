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

#ifndef ELIX_CORE_HPP
#define ELIX_CORE_HPP

// Standard Types
#ifdef PLATFORM_BEOS // GCC 2.9 - Unlikely to be supported
	#include <inttypes.h>
#else
	#include <stdint.h>
#endif


#define ASSERT(Expression) if(!(Expression)) {__builtin_trap();}
//#define ASSERT(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define UNUSEDARG __attribute__((unused))
#define MUSTFREEARG
#define REARG

#define ARRAYCOUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#define NULLIFY(f) if (f) { delete f; f = nullptr; }

#define PI32 3.14159265359f
#define PI64 3.14159265358979323846
#define TAU32 6.28318530717958647692f
#define DEG2RAD(a) a * 0.01745329251994329576923690768489f

#if defined(PLATFORM_WINDOWS) && defined(COMPILER_MSCV)
#define pZD "%Id"
#define pZX "%Ix"
#define pZU "%Iu"
#else
#define pZD "%zd"
#define pZX "%zx"
#define pZU "%zu"
#endif


typedef void* data_pointer; // uintptr_t

union elix_colour {
	uint32_t hex;
	struct { // Note: CLANG  -Wno-gnu-anonymous-struct
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};
	struct { // Note: CLANG  -Wno-gnu-anonymous-struct
		uint32_t rgb:24;
		uint32_t x:8;
	};
};
union elix_sv32_2 { struct{int32_t x; int32_t y;}; struct{int32_t width; int32_t height;}; };
union elix_uv32_2 { struct{uint32_t x; uint32_t y;}; struct{uint32_t width; uint32_t height;}; };
struct elix_sv16_2 { int16_t x; int16_t y; };
struct elix_uv16_2 { uint16_t x; uint16_t y; };

struct elix_graphic_data {
	union {
		uint8_t * data;
		uint32_t * pixels;
	};
	uint32_t width;
	uint32_t height;
	uint64_t size = 0;
	uint64_t pixel_count = 0;
	uint32_t ref = 1;
	uint8_t bpp = 4;
	uint8_t format = 0;
	uint16_t _unsued = 0;

};

struct elix_string {
	uint16_t length;
	uint16_t allocated;
	uint8_t * data;
};

struct elix_databuffer {
	uint64_t size = 0;
	uint8_t * data = nullptr;
};

struct elix_allocated_buffer {
	uint8_t data[1024];
	uint16_t data_size = 1024;
	uint16_t actual_size = 0;
};

struct elix_path {
	char * path = nullptr;
	char * filename = nullptr;
	char * filetype = nullptr;
};

#ifndef ELIX_SKIP_CORE_LOG
	#include <stdio.h>
	#define LOG_OUTPUT_ERROR stderr
	#define LOG_OUTPUT_MESSAGE stdout

	#define NAMEDLOG_MESSAGE(N, M, ...) printf("%23s | " M "\n", N, ##__VA_ARGS__)
	#define LOG_INFO(M, ...) printf( M "\n", ##__VA_ARGS__)
	#define LOG_MESSAGE(M, ...) printf("%24s:%04d | " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
	#define LOGF_MESSAGE(M, ...) printf("%21s() | " M "\n", __func__, ##__VA_ARGS__)
	#define LOG_ERROR(M, ...) fprintf(LOG_OUTPUT_ERROR, "%18s:%04d | " M "\n",__FILE__, __LINE__, ##__VA_ARGS__)
	#define LOGF_ERROR(M, ...) fprintf(LOG_OUTPUT_ERROR, "%21s() | " M "\n", __func__, ##__VA_ARGS__)
#else
	#define NAMEDLOG_MESSAGE(N,M, ...)
	#define LOG_INFO(M, ...)
	#define LOG_MESSAGE(M, ...)
	#define LOG_ERROR(M, ...)
	#define LOGF_MESSAGE(M, ...)
	#define LOGF_ERROR(M, ...)
#endif // ELIX_SKIP_CORE_LOG


inline elix_graphic_data * elix_graphic_data_create(elix_uv32_2 dimensions, uint8_t bpp = 4, UNUSEDARG uint8_t format = 0) {
	elix_graphic_data * buffer = new elix_graphic_data;

	buffer->width = dimensions.width;
	buffer->height = dimensions.height;
	buffer->bpp = bpp;
	buffer->pixel_count = buffer->width * buffer->height;
	buffer->size = buffer->pixel_count * buffer->bpp;
	buffer->pixels = new uint32_t[buffer->pixel_count];
	return buffer;
}

inline void elix_graphic_data_ref(elix_graphic_data * buffer) {
	buffer->ref++;
}

inline elix_graphic_data * elix_graphic_data_destroy(elix_graphic_data * buffer) {
	buffer->ref--;
	if ( !buffer->ref ) {
		NULLIFY(buffer->pixels);
		buffer->pixel_count = buffer->size = 0;
		NULLIFY(buffer);
	}
	return buffer;
}



inline void elix_databuffer_free(elix_databuffer * data) {
	NULLIFY(data->data);
	data->size = 0;
}


#endif // ELIX_CORE_HPP
