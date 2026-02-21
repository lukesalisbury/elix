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

#include "elix_file.h"


file_size elix_file__update_length(elix_file * file) {
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
		if ( file->pos >= file->length )
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

size_t elix_file_read_line( elix_file * file, char * string, size_t string_size) {
	if ( !file || !file->handle || file->length < 1 ) {
		return 0;
	}

	uint8_t value = 0;
	file_size pos = elix_file_tell(file);
	size_t read_counter = 0;
	while ( file->length > pos && read_counter < string_size ) {
		fread( &value, sizeof(uint8_t), 1, FH(file->handle) );
		if ( feof( FH(file->handle) ) == 0 || value == 0 ) {
			return read_counter;
		} else if ( value == 10 ) {
			return read_counter;
		} else {
			string[read_counter] = value;
			read_counter++;
		}
		
		pos++;
	}
	string[read_counter] = 0;
	return read_counter;
}
/*
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
*/

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

size_t elix_file_write( elix_file * file, data_pointer data, size_t size ) {
	if ( !file || !file->handle ) {
		return 0;
	}
	return fwrite(data, size, 1, FH(file->handle));

}

uint16_t elix_file_read_host16(elix_file * file) {
	uint16_t t = 0;
	elix_file_read(file, &t, 2, 1);
	return elix_endian_host16(t);
}

uint32_t elix_file_read_host32(elix_file * file) {
	uint32_t t = 0;
	elix_file_read(file, &t, 4, 1);
	return elix_endian_host32(t);
}

file_size elix_file_tell(elix_file * file) {
	return (file_size)(elix_file_offset(file));
}

char * elix_file_get_content(const char * filename, file_size * size) {
	char * content = nullptr;
	elix_file file;
	if ( elix_file_open(&file, filename, EFF_FILE_READ, nullptr) ) {
		content = ALLOCATE(char, file.length);

		elix_file_read(&file, content, file.length, 1);
		*size = file.length;
		elix_file_close(&file);
	}

	return content;
}

uint8_t * elix_file_read_content(elix_file * file, file_size * size) {
	uint8_t * content = ALLOCATE(uint8_t, file->length);

	elix_file_read(file, content, file->length, 1);
	*size = file->length;
	return content;
}


char * elix_file_read_string(elix_file * file, size_t data_size, uint8_t include_terminator) {
//	if ( !file || !file->handle  || !data_size ) {
//		return nullptr;
//	}

	char * string = ALLOCATE(char, data_size+include_terminator);
	fread(string, 1, data_size, FH(file->handle));
	return string;
}

size_t elix_file_write_string( elix_file * file, const char * string, size_t data_size) {
	if ( !file || !file->handle ) {
		return 0;
	}
	return fwrite(string, data_size, 1, FH(file->handle));
}

elix_file * elix_file_new(const char * filename) {
	elix_file * file = ALLOCATE(elix_file, 1);
	elix_file_open(file, filename,EFF_FILE_WRITE, nullptr);
	return file;
}



