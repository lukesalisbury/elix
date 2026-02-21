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

//TODO: Add Memory Mapped support

#ifndef ELIX_FILE_HEADER
#define ELIX_FILE_HEADER

#include "elix_core.h"
#include "elix_consent.h"
#include "elix_endian.h"

#define FH(q) (FILE *)q

typedef void * file_pointer; // uintptr_t

typedef uint64_t file_size; //NOTE: Some system may not support 2GB+ files, so it could be chnaged to reduce memory
typedef int64_t file_offset;

typedef enum {
	EFF_STATUS_UNKNOWN = 0x001,
	EFF_STATUS_WRITABLE = 0x002,
	EFF_STATUS_READABLE = 0x004,

	EFF_FILE_READ = 0x010, 
	EFF_FILE_WRITE = 0x020,  // Append
	EFF_FILE_TRUNCATE = 0x040, // Reset
	EFF_FILE_WRITING = 0x060,  
	EFF_FILE_NEW = 0x080,  // New
	EFF_FILE_CREATING = 0x0C0,  

	EFF_FILE_ERROR = 0x100,
	EFF_FILE_READ_ERROR = 0x300,
	EFF_FILE_WRITE_ERROR = 0x500,
	EFF_FILE_CREATE_ERROR = 0x700,

} ElixFileFlag;

struct ElixFile {
	FILE * handle;
	file_size length;
	file_size pos;
	uint32_t flag;
	void (*errorCallback)(const char * message);
};
typedef struct ElixFile elix_file;


#ifdef __cplusplus
extern "C" {
#endif

bool elix_file_open(elix_file * file, const char * filename, ElixFileFlag mode, elix_consent * consent);
elix_file * elix_file_new(const char * filename);
bool elix_file_close(elix_file * file);

bool elix_file_seek(elix_file * file, file_offset pos );
file_offset elix_file_offset(elix_file * file);
file_size elix_file_tell(elix_file * file);
bool elix_file_at_end(elix_file * file);

size_t elix_file_read(elix_file * file, data_pointer buffer, size_t data_size, size_t amount);
char * elix_file_read_string(elix_file * file, size_t data_size, uint8_t include_terminator);

uint16_t elix_file_read_host16(elix_file * file);
uint32_t elix_file_read_host32(elix_file * file);
size_t elix_file_read_line( elix_file * file, char * string, size_t string_size);
uint8_t * elix_file_read_content(elix_file * file, file_size * size);
char * elix_file_get_content(const char * filename, file_size * size);

size_t elix_file_write( elix_file * file, data_pointer data, size_t size );
size_t elix_file_write_string( elix_file * file, const char * string, size_t data_size);


#ifdef __cplusplus
}
#endif



#endif // ELIX_FILE_HEADER
