#include "elix_package.hpp"
#include "elix_file.hpp"


#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
#define MINIZ_NO_STDIO 1
#define MINIZ_NO_ARCHIVE_APIS 1
#define MINIZ_NO_ARCHIVE_WRITING_APIS 1
#include "miniz.c"


static uint8_t elix_package_magics[EP_AUTO][8] {
	{138, 'M', 'o', 'k', 'o', 'i', '1', 10},//EP_RESOURCES,
	{134, 'M', 'o', 'k', 'o', 'i', '2', 10},//EP_GAME,
	{137, 'M', 'o', 'k', 'o', 'i', '1', 10},//EP_GAME_OLD,
	{139, 'M', 'o', 'k', 'o', 'i', '1', 10} //EP_PATCH
};



inline void elix_package_stored_file__destroy(elix_package_stored_file * stored) {
	NULLIFY(stored->name);
	NULLIFY(stored->raw.data);
	NULLIFY(stored->compressed.data);
}



uint16_t elix_package__scan( elix_package * pkg ){
	uint16_t count = 0;
	uint16_t name_length = 0;
	do {
		name_length = elix_file_read_host16( &pkg->file );
		if ( name_length < 3 ) {
			if ( name_length )
				LOG_MESSAGE("Invalid File Name in package.");
			break;
		}

		elix_package_stored_file * info = new elix_package_stored_file();

		info->name = elix_file_read_string(&pkg->file, name_length, 1);
		info->compression_type = 'g';

		info->raw.hash = 0;
		info->raw.size = elix_file_read_host32( &pkg->file );
		info->raw.data = nullptr;

		info->compressed.hash = 0;
		info->compressed.size = elix_file_read_host32( &pkg->file );
		info->compressed.data = nullptr;

		info->file_offset = elix_file_offset(&pkg->file);
		elix_file_seek( &pkg->file, info->file_offset + info->compressed.size);

		elix_hashmap_insert(&pkg->files, info->name, info);

		count++;
		if ( elix_file_at_end(&pkg->file) )
			break;

	} while ( !elix_file_at_end(&pkg->file) );
	return count;
}


bool elix_package__read_header_oldgame(elix_package * pkg) {

	elix_file_read(&pkg->file, &pkg->header.oldgame.name, 1, 255);

	pkg->header.oldgame.id = elix_file_read_host32( &pkg->file );
	pkg->header.oldgame.logo_length = elix_file_read_host32( &pkg->file );
	if ( pkg->header.oldgame.logo_length ) {
		file_offset current = elix_file_offset(&pkg->file);
		elix_file_seek( &pkg->file, current + pkg->header.oldgame.logo_length);
	}

	pkg->header.oldgame.crc = elix_file_read_host32( &pkg->file );

	return true;
}


bool elix_package__read_header(elix_package * pkg, elix_package_type type ) {

	if ( elix_file_read(&pkg->file, pkg->magic, 1, 8) != 8) {
		return 0;
	}

	if ( type == EP_AUTO ) {
		for (uint8_t q = EP_RESOURCES;q < EP_AUTO; q++) {
			if ( memcmp(pkg->magic, elix_package_magics[type], 6) ==0 ) {
				LOG_MESSAGE("Package type: %d", type);
				pkg->header_type = type;
				switch (pkg->header_type) {
					case EP_GAME_OLD:
						elix_package__read_header_oldgame(pkg);
					break;
					default:
						LOG_MESSAGE("Not Supported Yet");
					break;
				}

				return 1;
			} else {
				LOG_MESSAGE("Not a Package of type: %d", type);
			}
		}
	} else if ( type >= EP_RESOURCES && type < EP_AUTO ) {
		if ( memcmp(pkg->magic, elix_package_magics[type], 6) ==0 ) {
			pkg->header_type = type;
			elix_package__read_header_oldgame(pkg);
			return 1;
		} else {
			LOG_MESSAGE("Not a Package of type: %d", type);
		}
	} else {
		LOG_MESSAGE("Unknown Package type");
		return 0;
	}

	return 0;

}


elix_package * elix_package_create( const char * filename, elix_package_type type ) {

	LOG_MESSAGE("Loading %s as %u", filename, type);

	elix_package * pkg = new elix_package();

	if ( elix_file_open(&pkg->file, filename, EFF_FILE_OPEN) ) {
		if ( elix_package__read_header(pkg, type )) {
			elix_package__scan(pkg);
		}
	}



	return pkg;

}

void  elix_hashmap_clear(elix_hashmap * hm, void (*delete_func)(data_pointer*));

void elix_package_stored_file__destroy(data_pointer * data) {
	elix_package_stored_file * info = (elix_package_stored_file*) *data;

	if ( info ) {
		if ( info->compressed.data )
			delete info->compressed.data;
		if ( info->raw.data )
			delete info->raw.data;
	}
	delete info;

	*data = nullptr;

}

void elix_package_destroy( elix_package * pkg ) {
	elix_hashmap_clear(&pkg->files, elix_package_stored_file__destroy);
	elix_file_close(&pkg->file);

}

size_t elix_package_info__print( elix_hashmap * hm ) {
	size_t q  = 0;
	data_pointer * current = &hm->active.values[0];
	for (size_t c = 0; c< ELIX_HASHMAP_POOL_SIZE; c++, current++ ) {
		if ( current && hm->active.keys[c] ) {
			q++;
			elix_package_stored_file * info = (elix_package_stored_file * )*current;
			LOG_MESSAGE("File: %s - %u compressed to %u", info->name, info->raw.size, info->compressed.size);
		}
	}
	if ( hm->next ) {
		q += elix_package_info__print(hm->next);
	}
	return q;
}

void elix_package_info( elix_package * pkg ) {
	if ( pkg ) {
		LOG_MESSAGE("Magic: M:%d V:%c", pkg->magic[0], pkg->magic[6]);

		switch (pkg->header_type) {
			case EP_GAME_OLD:
				LOG_MESSAGE("Title: %s [%zx] - CRC: %u Logo Size: %u", pkg->header.oldgame.name, pkg->header.oldgame.id , pkg->header.oldgame.crc, pkg->header.oldgame.logo_length);
			break;
			default:
				LOG_MESSAGE("Unknown Header Type");
			break;
		}

		size_t fc = elix_package_info__print(&pkg->files);
		LOG_MESSAGE("Files: %zu", fc);
	}
}

bool elix_package_file_uncompressed(elix_package_stored_file * info, elix_package_data * buffer) {

	switch (info->compression_type) {
		case 'g': {
			if ( buffer->data &&  info->compressed.data && info->raw.size == buffer->size) {
				mz_ulong writtrn = buffer->size;
				return mz_uncompress(buffer->data, &writtrn, info->compressed.data, info->compressed.size) == MZ_OK;
			}
			break;
		}
		default: {

		}
	}

	return false;
}




elix_package_data elix_package_get_file( elix_package * pkg, const char * file ){
	elix_package_data buffer = {0, 0, nullptr};
	//elix_file_seek(&pkg->file, 0);
	elix_package_stored_file * info = (elix_package_stored_file *)elix_hashmap_value(&pkg->files, file);
	if ( info ) {
		if ( info->raw.size ) {
			buffer.size = info->raw.size;
			buffer.data = new uint8_t[buffer.size]();

			if ( info->raw.data ) {
				memcpy(buffer.data, info->raw.data, buffer.size);

			} else if ( info->compressed.size ) {
				if ( !info->compressed.data ) {
					info->compressed.data = new uint8_t[info->compressed.size]();
					elix_file_seek(&pkg->file, info->file_offset);
					elix_file_read(&pkg->file, info->compressed.data, info->compressed.size, 1);
				}

				elix_package_file_uncompressed(info, &buffer);
			}
		}



	}
	return buffer;
}


