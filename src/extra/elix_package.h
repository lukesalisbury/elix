#ifndef ELIX_PACKAGE_HPP
#define ELIX_PACKAGE_HPP

#include "elix_core.h"
#include "elix_file.h"
#include "elix_hashmap.h"
/*
Package
- Type info
- Type header
- Header
 - publickeyLength [u16]
 - publickey [u8:var]
- Files Count [u16]
- Files List (Optional)
 - NameHash [u64]
 - Offset [u64]
- Files
 - NameLength [u8]
 - Name [u8:var]
 - CompressType [u8]
 - CompressLength [u32]
 - ExpandedLength [u32]
 - ExpandedDataHash [u64]
 - Data [u8:var]
*/

typedef enum elix_package_type {
	EP_RESOURCES,
	EP_GAME,
	EP_GAME_OLD,
	EP_PATCH,
	EP_AUTO
} elix_package_type;


typedef struct elix_package_data {
	uint64_t hash;
	uint32_t size;
	uint8_t * data;
} elix_package_data;

typedef struct elix_package_stored_file {
	char * name;
	char * path;
	uint8_t path_type;
	uint64_t file_offset;
	uint64_t name_hash;
	uint8_t compression_type;
	elix_package_data compressed;
	elix_package_data raw;
} elix_package_stored_file;

typedef struct elix_package_game {
	uint8_t name[256];
	uint64_t id;
	elix_colour logo_palette[64];
	uint8_t logo[64][64];
	uint64_t hash;
} elix_package_game;

typedef struct elix_package_oldgame {
	int8_t name[256];
	uint64_t id;
	uint32_t logo_length;
	uint8_t * logo;
	uint32_t crc;
} elix_package_oldgame;

typedef elix_package_stored_file * elix_package_stored_file_array;

typedef struct elix_package {
	uint8_t magic[8];
	elix_file file;
	elix_hashmap files;
	uint8_t header_type;
	union {
		elix_package_oldgame oldgame;
		elix_package_game game;
	} header;
} elix_package;

typedef struct elix_assets_file_info {
	elix_package * parent_package;
} elix_assets_file_info;

typedef elix_assets_file_info * elix_assets_file_info_array;
typedef elix_package * elix_package_array;

typedef struct elix_assets {

	//
	elix_assets_file_info_array * files;
	elix_package_array * loaded_packages;
} elix_assets;


#ifdef __cplusplus
extern "C" {
#endif
elix_package * elix_package_create( const char * filename, elix_package_type type );
void elix_package_destroy( elix_package * pkg );

void elix_package_info( elix_package * pkg );

elix_package_data elix_package_get_file( elix_package * pkg, const char * file );

#ifdef __cplusplus
}
#endif


#endif
