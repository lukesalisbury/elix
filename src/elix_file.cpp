#include "elix_file.hpp"
#include "stretchy_buffer.h"

inline file_size elix_file__update_length(elix_file * file) {
	file_offset pos = elix_file_offset(file);
	fseeko64( FH(file->handle), 0, SEEK_END );
	file->length = elix_file_tell(file);
	fseeko64( FH(file->handle), pos, SEEK_SET );
	return file->length;
}

bool elix_file_open(elix_file * file, const char * filename, ElixFileFlag mode, UNUSEDARG elix_consent *  consent) {
	//TODO: Add Permissions Checks
	file->handle = fopen(filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	if ( file->handle ) {
		file->flag = mode;
		elix_file__update_length(file);
		return true;
	}
	file->flag = (mode | EFF_FILE_WRITE ? EFF_FILE_WRITE_ERROR : EFF_FILE_READ_ERROR);
	return false;

};
bool elix_file_close(elix_file * file) {
	if ( file->handle )	{
		return fclose( FH(file->handle) ) == 0;
	}
	return false;
}

file_offset elix_file_offset(elix_file * file) {
	// Note: due to pointless warnings, using ftello64 instead of ftello
	file_offset q = ftello64( FH(file->handle) );
	if ( q < 0 ) {
		return 0;
	}
	return q;
}

bool elix_file_at_end(elix_file * file) {
	if ( file->handle ) {
		if ( file->pos == file->length )
			return true;
		return !(feof( FH(file->handle) ) == 0);
	}
	return true;
}

bool elix_file_seek(elix_file * file, file_offset pos ) {
	if ( fseeko64( FH(file->handle), pos, SEEK_SET ) == 0) {
		file->pos = pos;
		return true;
	}
	return false;
}

size_t elix_file_read(elix_file * file, data_pointer buffer, size_t data_size, size_t amount) {
	if ( !file || !file->handle || file->length < data_size || !buffer ) {
		return 0;
	}

	size_t q = fread(buffer, data_size, amount, FH(file->handle));
	if ( q != 0 ) {
		file_offset p = ftello64( FH(file->handle) );
		if ( p >= 0 ) {
			file->pos = p;
		}
	}
	//ASSERT(q!=0)
	return q;

}

size_t elix_file_readline( elix_file * file, char * string) {
	if ( !file || !file->handle || file->length < 1 ) {
		return 0;
	}
	stb__sbgrow(string, 128);
	uint8_t value = 0;
	file_size pos = elix_file_tell(file);

	while ( file->length > pos ) {
		fread( &value, sizeof(uint8_t), 1, FH(file->handle) );
		if ( feof( FH(file->handle) ) == 0 || value == 0 ) {
			return 0;
		} else if ( value == 10 ) {
			return stb_sb_count(string);
		} else {
			stb_sb_push(string, value);
		}
		pos++;
	}
	return stb_sb_count(string);
}


file_offset elix_file_scan( elix_file * file, file_offset startPostion, uint8_t * needle, uint16_t needleLength ) {
	file_offset position = -1;
	bool needle_found = false;
	if ( file->handle && needleLength > 0 )
	{
		int32_t c = 0;
		uint8_t check = 0;

		elix_file_seek(file, startPostion);
		do {
			c = fgetc( FH(file->handle) );
			check = c;

			if ( check == needle[0] ){
				uint16_t scan_count = 1;

				position = elix_file_tell(file) - 1; //want to go back to the first character

				while ( scan_count < needleLength ) {
					c = fgetc( FH(file->handle) );
					check = c;

					if ( check != needle[scan_count] ) {
						position = -1;
						scan_count = needleLength;
					}
					scan_count++;
				}

				if ( position > 0 ) {
					needle_found = true;
				}
			}
		} while ( !elix_file_at_end(file) && !needle_found );

	}

	elix_file_seek(file, 0);
	return position;
}

