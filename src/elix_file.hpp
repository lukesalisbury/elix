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


#ifndef ELIX_FILE_HPP
#define ELIX_FILE_HPP

#include "elix_core.h"
#include "elix_consent.hpp"
#include "elix_endian.hpp"

#define FH(q) static_cast<FILE *>(q)


#ifndef ELIX_FILE_PATH_LENGTH
	#define ELIX_FILE_PATH_LENGTH 768
#endif

typedef void* file_pointer; //uintptr_t

typedef uint64_t file_size; //NOTE: Some system may not support 2GB+ files, so it could be chnaged to reduce memory
typedef int64_t file_offset;

typedef enum {
	EFF_STATUS_UNKNOWN = 1 << 0,
	EFF_FILE_OPEN = 1 << 1, EFF_FILE_READ_ONLY = 1 << 2, EFF_FILE_READ_ERROR = 1 << 3 ,
	EFF_FILE_WRITE = 1 << 4, EFF_FILE_WRITE_ERROR = 1 << 5, EFF_WRITABLE = 1 << 6
} ElixFileFlag;

struct elix_file {
	file_pointer handle;
	file_size length = 0;
	file_size pos = 0;
	uint32_t flag = EFF_STATUS_UNKNOWN;

	void (*errorCallback)(const char * message) = nullptr;
};

bool elix_file_open(elix_file * file, const char * filename, ElixFileFlag mode = EFF_FILE_READ_ONLY, elix_consent * consent = nullptr);
bool elix_file_close(elix_file * file);

file_offset elix_file_offset(elix_file * file);
size_t elix_file_read_line( elix_file * file, char * string);
size_t elix_file_read(elix_file * file, data_pointer buffer, size_t data_size, size_t amount);

bool elix_file_at_end(elix_file * file);

inline file_size elix_file_tell(elix_file * file) {
	return static_cast<file_size>(elix_file_offset(file));
}

bool elix_file_seek(elix_file * file, file_offset pos );

inline uint8_t * elix_file_read_content(elix_file * file, file_size & size) {
	uint8_t * content = new uint8_t[file->length];

	elix_file_read(file, content, file->length, 1);
	size = file->length;
	return content;
}

inline uint16_t elix_file_read_host16(elix_file * file) {
	uint16_t t = 0;
	elix_file_read(file, &t, 2, 1);
	return elix_endian_host16(t);
}

inline uint32_t elix_file_read_host32(elix_file * file) {
	uint32_t t = 0;
	elix_file_read(file, &t, 4, 1);
	return elix_endian_host32(t);
}

inline char * elix_file_read_string(elix_file * file, size_t data_size, uint8_t include_terminator = 0) {
//	if ( !file || !file->handle  || !data_size ) {
//		return 0;
//	}
	char * string = new char[data_size+include_terminator]();

	fread(string, 1, data_size, FH(file->handle));

	return string;
}




#endif // ELIX_FILE_HPP
