#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#ifndef nullptr
	#define nullptr NULL
#endif

#define LOG_MESSAGE(M, ...) printf("%18s:%04d | " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(M, ...) printf( M "\n", ##__VA_ARGS__)
#define ARRAY_SIZE(Array) (sizeof(Array) / sizeof((Array)[0]))
#define FH(q) q
#define UNUSEDARG __attribute__((unused))

#ifndef ELIX_FILE_PATH_LENGTH
	#define ELIX_FILE_PATH_LENGTH 768
#endif

#ifndef ELIX_FILE_NAME_LENGTH
	#define ELIX_FILE_NAME_LENGTH 256
#endif

#ifndef ftello64
	#define ftello64 ftello
#endif
#ifndef fseeko64
	#define fseeko64 fseeko
#endif

#define ELIX_CHAR_LOWER 1
#define ELIX_CHAR_UPPER 2
#define ELIX_CHAR_CAPITALISE 3

typedef void * data_pointer;
typedef FILE * file_pointer;

typedef uint64_t file_size;
typedef int64_t file_offset;

typedef enum {
	EFF_STATUS_UNKNOWN = 1 << 0,
	EFF_FILE_OPEN = 1 << 1, EFF_FILE_READ_ONLY = 1 << 2, EFF_FILE_READ_ERROR = 1 << 3 ,
	EFF_FILE_WRITE = 1 << 4, EFF_FILE_WRITE_ERROR = 1 << 5, EFF_WRITABLE = 1 << 6
} ElixFileFlag;

struct ElixFile {
	FILE * handle;
	file_size length;
	file_size pos;
	uint32_t flag;
};
typedef struct ElixFile elix_file;

uint32_t elix_hash( const char * str, size_t len ) {
	//Jenkins One-at-a-time hash
	uint32_t hash = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		hash += str[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

bool elix_os_directory_make(const char * path, uint32_t mode, UNUSEDARG bool parent) {
	#if __WIN32__
		if ( ! mkdir(path) ) {
			return true;
		}

		errno_t err;
		_get_errno( &err );
	#else
		int32_t err = mkdir(path, mode);
		if ( !err ) {
			return true;
		}
	#endif

	if ( err == EEXIST ) {
		//LOG_MESSAGE("Directory was not created because dirname is the name of an existing file, directory, or device. %s", path);
	} else if ( err == ENOENT ) {
		//LOG_MESSAGE("Path was not found. %s", path);
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

uint32_t elix_file_at_end(elix_file * file) {
	if ( file->handle ) {
		if ( file->pos >= file->length )
			return true;
		return !(feof( FH(file->handle) ) == 0);
	}
	return true;
}

uint32_t elix_file_seek(elix_file * file, file_offset pos ) {
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
		file_offset p = elix_file_offset( file );
		if ( p >= 0 ) {
			file->pos = p;
		}
	}
	return q;
}

file_size elix_file_tell(elix_file * file) {
	return (file_size)(elix_file_offset(file));
}

file_size elix_file__update_length(elix_file * file) {
	file_offset pos = elix_file_offset(file);
	fseeko64( FH(file->handle), 0, SEEK_END );
	file->length = elix_file_tell(file);
	fseeko64( FH(file->handle), pos, SEEK_SET );
	return file->length;
}

uint32_t elix_file_open(elix_file * file, const char * filename, ElixFileFlag mode) {
	file->pos = 0;
	file->length = 0;
	file->handle = fopen(filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	//fopen_s( &file->handle, filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	if ( file->handle ) {
		file->flag = mode;
		elix_file__update_length(file);
		return true;
	} else {
		//LOG_MESSAGE("File not found %s", filename);
	}
	file->flag = (mode | EFF_FILE_WRITE ? EFF_FILE_WRITE_ERROR : EFF_FILE_READ_ERROR);
	return false;

}

uint32_t elix_file_close(elix_file * file) {
	if ( file->handle ) {
		return fclose( FH(file->handle) ) == 0;
	}
	return false;
}

size_t elix_cstring_length(const char * string, uint8_t include_terminator ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}

bool elix_cstring_has_suffix( const char * str, const char * suffix) {
	size_t str_length = elix_cstring_length(str, 0);
	size_t suffix_length = elix_cstring_length(suffix, 0);
	size_t offset = 0;

	if ( str_length < suffix_length )
		return false;

	offset = str_length - suffix_length;
	for (size_t c = 0; c < suffix_length;  c++ ) {
		if ( str[offset + c] != suffix[c] )
			return false;
	}
	return true;
}

bool elix_cstring_has_prefix( const char * str, const char * prefix) {
	size_t str_length = elix_cstring_length(str, 0);
	size_t prefix_length = elix_cstring_length(prefix, 0);

	if ( str_length < prefix_length )
		return false;

	for (size_t c = 0; c < prefix_length;  c++ ) {
		if ( str[c] != prefix[c] )
			return false;
	}
	return true;
}

size_t elix_cstring_append( char * str, const size_t len, const char * text, const size_t text_len) {
	size_t length = elix_cstring_length(str, 0);
	// TODO: Switch to memcpy
	for (size_t c = 0;length < len && c < text_len; length++, c++) {
		str[length] = text[c];
	}
	str[length+1] = 0;
	return length;
}

void elix_cstring_copy( const char * source_init, char * dest_init) {
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
	} while(*source != 0);
	*dest = '\0';
}

char * elix_cstring_from( const char * source, const char * default_str, size_t max_length ) {
	//ASSERT( default_str != nullptr )
	const char * ptr = source != nullptr ? source : default_str;
	size_t length = (max_length == SIZE_MAX) ? elix_cstring_length(ptr, 1) : max_length;

	if ( length <= 1 ) {
		//LOG_ERROR("elix_cstring_from failed, source was empty");
		return nullptr;
	}

	char * dest = malloc(length); //new char[length]();
	elix_cstring_copy(ptr, dest);
	dest[length] = 0;
	return dest;
}

size_t elix_cstring_trim( char * string ) {
	size_t length = elix_cstring_length(string, 0);

	while( length && string[0] == ' ' ) {
		memmove(string, string + 1, length);
		length--;
	}
	size_t pos = length ? length-1 : 0;
	while( pos && string[pos] == ' ' ) {
		string[pos] = 0;
		length--;
		pos--;
	}
	return length;
}

void elix_cstring_transform( char * string, uint8_t mode ) {
	size_t length = elix_cstring_length(string, 0);
	for (size_t var = 0; var < length; ++var) {
		switch (mode) {
			case ELIX_CHAR_LOWER:
				if (string[var] >= 'A' && string[var] <= 'Z') {
					string[var] += 32;
				}
				break;
			case ELIX_CHAR_UPPER:
				if (string[var] >= 'a' && string[var] <= 'z') {
					string[var] -= 32;
				}
				break;
			case ELIX_CHAR_CAPITALISE:
				if (string[var] >= 'a' && string[var] <= 'z') {
					string[var] -= 32;
				}
				mode = 0;
				break;
			default:
				break;
		}
	}
}

size_t elix_cstring_find_of(const char * str, const char * search, size_t offset) {
	size_t length = elix_cstring_length(str,0);
	size_t sl = elix_cstring_length(search,0);
	for (size_t c = offset; c < length - (sl-1) && str[c] != 0; c++) {
		bool found = false;
		for (size_t sc = 0; sc < sl; sc++) {
			if ( str[c+sc] == search[sc]) {
				found = (sc == sl-1);
			} else {
				break;
			}
		}
		if ( found )
			return c;
	}
	return SIZE_MAX;
}

size_t elix_cstring_inreplace( char * source_text, size_t buffer_size, const char *search, const char *replace) {
	//NOTE: This is slow, and not a good implemenation
	//TODO: Check for overflow
	size_t search_len = 0, replace_len = 0;
	ssize_t diff_len = 0;
	size_t source_len = elix_cstring_length(source_text, 1);
	size_t pos = elix_cstring_find_of(source_text, search, 0);
	if ( pos != SIZE_MAX ) {
		search_len = elix_cstring_length(search, 0);
		replace_len = elix_cstring_length(replace, 0);
		diff_len = search_len - replace_len;

		size_t l = 0;
		if ( diff_len > 0 ) {
			for (size_t i = pos; i < source_len; l++,i++) {
				if ( l < replace_len ) {
					source_text[i] = replace[l];
				} else {
					source_text[i] = source_text[i+diff_len];
				}
			}
		} else {
			if ( pos+search_len >= buffer_size) {
				return 0;
			}
			if ( diff_len < 0 ) {
				size_t j;
				for (size_t i = source_len; i > pos+search_len; i--) {
					j = i+diff_len;
					source_text[i] = source_text[j];
				}
			}
			for (size_t i = pos; i < source_len && l < replace_len; l++,i++) {
				source_text[i] = replace[l];
			}
		}


	}
	return diff_len;
}



struct ElixPath {
	char * uri;
	char * path;
	char * filename;
	char * filetype;
};
typedef struct ElixPath elix_path;

struct ElixDirectory {
	uint16_t count;
	elix_path * files;
};
typedef struct ElixDirectory elix_directory;

elix_path elix_path_create(const char * string) {
	elix_path uri = {0,0,0,0};
	size_t length = elix_cstring_length(string, 0);
	size_t split = SIZE_MAX;
	size_t extension_split = SIZE_MAX;
	for (split = length-1; split > 0; split--) {
		if ( string[split] == '\\' || string[split] == '/') {
			split++;
			break;
		}
		if ( extension_split == SIZE_MAX && string[split] == '.') {
			extension_split = split;
		}
	}

	//ASSERT(split < length);
	uri.uri = malloc(length+1);
	memset(uri.uri,0, length+1);
	elix_cstring_copy(string, uri.uri);
	uri.path = elix_cstring_from( string, "/", split );

#ifdef PLATFORM_WINDOWS
	elix_cstring_char_replace(uri.path, '\\', '/');
#endif
	if ( extension_split != SIZE_MAX ) {
		uri.filename = elix_cstring_from(string + split, "/", extension_split - split);
		uri.filetype = elix_cstring_from(string + extension_split, "/",SIZE_MAX);
	} else {
		uri.filename = elix_cstring_from(string + split, "/", length - split);
		uri.filetype = nullptr;
	}

	return uri;
}

void elix_os_directory_list_destroy(elix_directory ** directory) {
	for (int f = 0; f < (*directory)->count; ++f) {
		//free((*directory)->files[f]);
	}
	free((*directory)->files);
	//free(*directory);
	*directory = nullptr;
}

elix_directory * elix_os_directory_list_files(const char * path, const char * suffix) {
	elix_directory * directory = nullptr;
	DIR * current_directory;
	struct dirent * entity;
	current_directory = opendir(path);
	if ( !current_directory ) {
		return directory;
	}
	directory = calloc(1, sizeof(directory));
	while (entity = readdir(current_directory) ) {
		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)){

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) )
				directory->count++;
		} else {
			directory->count++;
		}
	}
	size_t path_len = elix_cstring_length(path, 0);
	directory->files = malloc(directory->count * sizeof(elix_path));
	rewinddir(current_directory);
	uint16_t index = 0;
	char buffer[ELIX_FILE_PATH_LENGTH] = {0};
	elix_cstring_copy(path, buffer);
	while (entity = readdir(current_directory) ) {
		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)){

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) ) {
				memset(buffer+path_len, 0, ELIX_FILE_PATH_LENGTH-path_len);
				elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, entity->d_name, elix_cstring_length(entity->d_name, 0));
				directory->files[index] = elix_path_create(buffer);
				index++;
			}
		} else {
			memset(buffer+path_len, 0, ELIX_FILE_PATH_LENGTH-path_len);
			elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, entity->d_name, elix_cstring_length(entity->d_name, 0));
			directory->files[index] = elix_path_create(buffer);
			index++;
		}
	}
	closedir(current_directory);
	return directory;
}

size_t elix_file_read_line(elix_file * file, data_pointer data, size_t data_size) {
	if ( !file || !file->handle || !data ) {
		return 0;
	}

	uint8_t char_buffer;
	uint8_t * data_buffer = data;
	size_t index = 0;
	size_t q;// = elix_file_read(file, &char_buffer, 1,1);

	do {
		q = elix_file_read(file, &char_buffer, 1,1);

		if (char_buffer > 13 )
			data_buffer[index++] = char_buffer;

	} while ( char_buffer > 10  && index < data_size && !elix_file_at_end(file));
	data_buffer[index] = 0;
	return index;
}

size_t elix_file_write_string( elix_file * file, const char * string, size_t data_size) {
	if ( !file || !file->handle ) {
		return 0;
	}
	if ( data_size == 0 ) {
		data_size = elix_cstring_length(string, 0);
	}
	return fwrite(string, data_size, 1, FH(file->handle));
}