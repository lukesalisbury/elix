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

#ifndef ELIX_CORE_H
#define ELIX_CORE_H

// Standard Types
#ifdef PLATFORM_BEOS // GCC 2.9 - Unlikely to be supported
	#include <inttypes.h>
#else
	#include <stdint.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#define ASSERT(Expression) if(!(Expression)) {__builtin_trap();}
//#define ASSERT(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define UNUSEDARG __attribute__((unused))
#define MUSTFREEARG
#define REARG

#define ARRAYCOUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

// memory management
#ifdef __cplusplus
//#define ALLOCATE(TYPE, COUNT) (COUNT > 1 ? new TYPE[COUNT]() : new TYPE())
//#define NULLIFY(f) if (f) { delete f; f = nullptr; }
#define ALLOCATE(TYPE, COUNT) (TYPE *)calloc(COUNT, sizeof(TYPE))
#define NULLIFY(f) if (f != nullptr) { free(f); f = nullptr; }
#else
#define nullptr NULL
#define ALLOCATE(TYPE, COUNT) (TYPE *)calloc(COUNT, sizeof(TYPE))
#define NULLIFY(f) if (f != nullptr) { free(f); f = nullptr; }
#endif


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


#ifndef SSIZE_MAX
	#ifdef _WIN64
		#define SSIZE_MAX INT64_MAX
		#define SSIZE_MIN INT64_MIN
	#else
		#define SSIZE_MAX INT32_MAX
		#define SSIZE_MIN INT32_MIN
	#endif
#endif

#ifndef __USE_LARGEFILE64
	#define __USE_LARGEFILE64
#endif

#ifndef ELIX_FILE_PATH_LENGTH
	#define ELIX_FILE_PATH_LENGTH 768
#endif
#ifndef ELIX_FILE_NAME_LENGTH
	#define ELIX_FILE_NAME_LENGTH 256
#endif


typedef void* data_pointer; // uintptr_t
typedef uint32_t bool32;
typedef uint16_t bool16;

typedef int32_t function_results;

typedef union elix_colour {
	uint32_t hex;
	struct { // Note: CLANG  -Wno-gnu-anonymous-struct
		#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
		#else
		uint8_t a;
		uint8_t b;
		uint8_t g;
		uint8_t r;
		#endif
	};
} elix_colour;
typedef union elix_sv32_2 { struct{int32_t x; int32_t y;}; struct{int32_t width; int32_t height;}; } elix_sv32_2;
typedef union elix_uv32_2 { struct{uint32_t x; uint32_t y;}; struct{uint32_t width; uint32_t height;}; } elix_uv32_2;
typedef union elix_v64_2 { struct{double x; double y;}; struct{double width; double height;}; } elix_v64_2;
typedef union elix_v32_2 { struct{float x; float y;}; struct{float width; float height;}; } elix_v32_2;
typedef struct elix_sv16_2 { int16_t x; int16_t y; } elix_sv16_2;
typedef struct elix_uv16_2 { uint16_t x; uint16_t y; } elix_uv16_2;


typedef struct elix_databuffer {
	uint8_t * data;
	uint64_t size;
} elix_databuffer;


#ifndef ELIX_SKIP_CORE_LOG
	#include <stdio.h>
	#define LOG_OUTPUT_ERROR stderr
	#define LOG_OUTPUT_MESSAGE stdout

	#define NAMEDLOG_MESSAGE(N, M, ...) printf("%23s | " M "\n", N, ##__VA_ARGS__)
	#define LOG_INFO(M, ...) printf( M "\n", ##__VA_ARGS__)

	#ifdef _DEBUG
		#define PRINT(M, ...) printf( M "\n", ##__VA_ARGS__)
	#else
		#define PRINT(M, ...) 
	#endif

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
	#define PRINT(M, ...) 
#endif // ELIX_SKIP_CORE_LOG


#define RESULTS_SUCCESS 					0x00
#define RESULTS_FAILED	 					0x01
#define RESULTS_ERROR						0x80
#define RESULTS_ERROR_ARGUMENT				0x81
#define RESULTS_MISSING_CONFIG				0x90
#define RESULTS_FUNCTION_UNIMPLEMENTED		0xFF

#define RESULTS_NOTFOUND 					0x01
#define RESULTS_FOUND 						0x00

#define RESULTS_PENDING						0x02


#endif // ELIX_CORE_H
