//#!gcc ./genscript.c -o ./gsb.exe
/*************************************************************************
 * Generates build.ninja files for multiple platforms
 * build: gcc genscript.c -o gsb.exe
 *        clang genscript.c -o gsb.exe
 * run: gsb.exe [-help]
 * 
 * 
 * Modules
 *  If first line is $static or $shared it will be build as such 
*************************************************************************/
//TODO: Fix Complier selection

#define GENSCRIPT_VERSION 20250705

#define SOURCE_DIRECTORY "src"
//#define SOURCE_DIRECTORY "source"
#define DEFAULT_BUILD_MODE "debug" //"release"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef SSIZE_MAX
	#ifdef _WIN64
		#define SSIZE_MAX INT64_MAX
		#define SSIZE_MIN INT64_MIN
	#else
		#define SSIZE_MAX INT32_MAX
		#define SSIZE_MIN INT32_MIN
	#endif
#endif

#define __USE_LARGEFILE64
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#if __WIN32__
#include <process.h>
#else
#include <spawn.h>
#endif

#ifndef nullptr
	#define nullptr NULL
#endif


/*** Elix ***/
#define free_if(M) if (M != nullptr) { free(M); M = nullptr;}

#define LOG_LINE printf("%s:%d \n", __FILE__, __LINE__)
#define LOG_MESSAGE(M, ...) printf("%18s:%04d | " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(M, ...) printf( M "\n", ##__VA_ARGS__)
#define PRINT(M, ...) printf( M , ##__VA_ARGS__)

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
	EFF_STATUS_UNKNOWN = 0x001,
	EFF_STATUS_WRITABLE = 0x002,
	EFF_STATUS_READABLE = 0x004,

	EFF_FILE_READ = 0x010, 
	EFF_FILE_WRITE = 0x020,  // Append
	EFF_FILE_TRUNCATE = 0x040, // Reset
	EFF_FILE_WRITING = 0x060,  
	EFF_FILE_NEW = 0x080,  //new
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
};
typedef struct ElixFile elix_file;

// > String
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
	str[length] = 0;
	return length;
}

void elix_cstring_copy( const char * source_init, char * dest_init) {
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
	} while( *source != 0);
	*dest = '\0';
}

void elix_cstring_copy2( const char * source_init, char * dest_init, size_t dest_size) {
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
		dest_size--;
	} while( dest_size && *source != 0);
	*dest = '\0';
}

char * elix_cstring_from( const char * source, const char * default_str, size_t max_length ) {
	//ASSERT( default_str != nullptr )
	const char * ptr = source != nullptr ? source : default_str;
	size_t length = (max_length == SIZE_MAX) ? elix_cstring_length(ptr, 1) : max_length;

	if ( length <= 1 ) {
		if ( default_str != nullptr ) {
			ptr = default_str;
			length = elix_cstring_length(ptr, 1);
		} else {
			//LOG_ERROR("elix_cstring_from failed, source was empty");
			return nullptr;
		}
	}

	char * dest = calloc(length, 1); //new char[length]();
	elix_cstring_copy2(ptr, dest, length-1);
	//dest[length] = 0;
	return dest;
}

#define ELIX_IS_WHITESPACE(x) (x == ' ' || x == '\n' || x == '\r')
size_t elix_cstring_trim( char * string ) {
	size_t length = elix_cstring_length(string, 0);

	while( length && ELIX_IS_WHITESPACE(string[0])  ) {
		memmove(string, string + 1, length);
		length--;
	}
	size_t pos = length ? length-1 : 0;
	while( pos && ELIX_IS_WHITESPACE(string[pos]) ) {
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

size_t elix_cstring_find_last_of(const char * str, const char * search, size_t offset) {
	size_t length = elix_cstring_length(str, 0);
	size_t sl = elix_cstring_length(search, 0);
	for (size_t c = length - sl; c > offset; c--) {
		bool found = false;
		for (size_t sc = 0; sc < sl; sc++) {
			if ( str[c+sc] == search[sc]) {
				found = (sc == sl-1);
			} else {
				break;
			}
		}
		if ( found ) {
			return c;
		}
	}
	return SIZE_MAX;
}

size_t elix_cstring_inreplace( char * source_text, size_t buffer_size, const char *search, const char *replace, uint8_t repeat) {
	//NOTE: This is slow, and not a good implemenation
	//TODO: Check for overflow
	//TODO: Implement Repeat
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
				size_t j = source_len-diff_len;
				for (size_t i = source_len; i >= pos+search_len; i--) {
					j = i-diff_len;
					source_text[j] = source_text[i];
				}
			}
			for (size_t i = pos; i < buffer_size && l < replace_len; l++,i++) {
				source_text[i] = replace[l];
			}
		}

	}
	return diff_len;
}

uint32_t elix_cstring_hash( const char * str ) {
	return elix_hash(str, elix_cstring_length(str, 0));
}

uint8_t elix_cstring_match( const char * p1, const char * p2,size_t max_length) {
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < max_length; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
		if ( a[i] == 0 ) {
			return 1;
		}
	}
	return 1;
}


char * elix_cstring_merge( char ** source_array, char token) {
	size_t str_size = 0;
	size_t str_position = 0;
	char * string = nullptr;
	uint8_t c = 0;
	uint8_t max = 0;

	if (!source_array) {
		return string;
	}

	while ( source_array[c]) {
		str_size += elix_cstring_length(source_array[c], 0 );
		c++;
	}
	max = c;

	if ( str_size > 0x1FF ) {
		return string;
	}

	str_size += c;
	string = calloc(str_size, sizeof(char));
	
	for (uint8_t i = 0; i < max; i++) {
		str_position = elix_cstring_append(string, str_size, source_array[i], elix_cstring_length(source_array[i], 1) );
		if ( str_position < str_size ) {
			string[str_position-1] = token;
		}
	}

	return string;
}

char ** elix_cstring_split( const char * source, char token, char string_bracket) {
	#define token_cache 8
	size_t source_len = elix_cstring_length(source, 0);
	size_t tokens = 0;
	size_t position[token_cache] = {0};
	size_t maxlength = 0, last_token = 0;
	uint8_t outside_string = 1;
	for (size_t i = 0; i < source_len; i++)	{
		if ( source[i] == token ) {
			if ( outside_string ) {
				maxlength = (i - last_token) > maxlength ? (i - last_token) : maxlength;
				last_token = i;
				if (tokens < token_cache ) {
					position[tokens] = last_token + 1;
				}
				tokens++;
			}
		} else if ( source[i] == string_bracket ) {
			outside_string = !outside_string;
		}
	}
	position[tokens] = source_len+1;
	tokens++;

	char ** output = malloc((tokens+1) * sizeof(char *));
	if (tokens > token_cache ) {
		//TODO
	} else {
		last_token = 0;
		for (size_t i = 0; i < tokens; i++)	{
			output[i] = elix_cstring_from(source + last_token, "", position[i] - last_token);
			//LOG_INFO("%d > %d - '%s'", last_token, position[i], output[i]);
			last_token = position[i];
		}
		output[tokens] = nullptr;
	}
	return output;
	#undef token_cache
}

size_t elix_cstring_dequote( char * string ) {
	size_t length = elix_cstring_length(string, 0);

	while( length && string[0] == '"' ) {
		memmove(string, string + 1, length);
		length--;
	}
	size_t pos = length ? length-1 : 0;
	while( pos && string[pos] == '"' ) {
		string[pos] = 0;
		length--;
		pos--;
	}
	return length;
}

// < String

// > File

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
	file->handle = 0;
	if ( mode & EFF_FILE_NEW ) {
		struct stat buffer = {0};
		if ( stat(filename, &buffer) == 0 ) {
			file->flag = EFF_FILE_CREATE_ERROR;
			return (uint32_t) EFF_FILE_CREATE_ERROR;
		}
	}

	//
	char cmode[4] = "rb";
	if ( mode & EFF_FILE_CREATING ) {
		cmode[0] = 'w';
	} else if ( mode & EFF_FILE_WRITING ) {
		cmode[0] = 'a';
	}
	file->handle = fopen(filename, cmode);

	//fopen_s( &file->handle, filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	if ( file->handle ) {
		file->flag = mode;
		elix_file__update_length(file);
		return 0;
	} else {
		file->flag = EFF_FILE_ERROR;
		//LOG_MESSAGE("File not found %s", filename);
	}
	return (uint32_t)(EFF_FILE_ERROR | (mode & EFF_FILE_WRITING ? EFF_FILE_WRITE_ERROR : EFF_FILE_READ_ERROR));

}

uint32_t elix_file_close(elix_file * file) {
	if ( file->handle ) {
		return fclose( FH(file->handle) ) == 0;
	}
	return false;
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


// < File

// > Path

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

void elix_path_update(elix_path * path, const char * string) {

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

	path->uri = calloc(length+1, 1);
	elix_cstring_copy(string, path->uri);
	path->path = elix_cstring_from( string, "/", split );


#ifdef PLATFORM_WINDOWS
	elix_cstring_char_replace(path->path, '\\', '/');
#endif
	if ( extension_split != SIZE_MAX && extension_split != split ) {
		path->filename = elix_cstring_from(string + split, "/", extension_split - split + 1);
		path->filetype = elix_cstring_from(string + extension_split, "/",SIZE_MAX);
	} else {
		path->filename = elix_cstring_from(string + split, "/", length - split+1);
		path->filetype = nullptr;
	}

}

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

	uri.uri = malloc(length+1);
	memset(uri.uri,0, length+1);
	elix_cstring_copy(string, uri.uri);
	uri.path = elix_cstring_from( string, "/", split );


#ifdef PLATFORM_WINDOWS
	elix_cstring_char_replace(uri.path, '\\', '/');
#endif
	if ( extension_split != SIZE_MAX && extension_split != split ) {
		uri.filename = elix_cstring_from(string + split, "/", extension_split - split + 1);
		uri.filetype = elix_cstring_from(string + extension_split, "/",SIZE_MAX);
	} else {
		uri.filename = elix_cstring_from(string + split, "/", length - split+1);
		uri.filetype = nullptr;
	}
	return uri;
}

// < Path

// > OS

void elix_os_directory_list_destroy(elix_directory ** directory) {
	for (int f = 0; f < (*directory)->count; ++f) {
		free_if((*directory)->files[f].uri)
		free_if((*directory)->files[f].path)
		free_if((*directory)->files[f].filename)
		free_if((*directory)->files[f].filetype)
	}
	(*directory)->count = 0;
	free_if((*directory)->files)
	//free_if(*directory)
}

elix_directory * elix_os_directory_list_files(const char * path, const char * suffix) {
	uint16_t index = 0;
	elix_directory * directory = nullptr;
	DIR * current_directory = nullptr;
	struct dirent * entity = nullptr;
	char buffer[ELIX_FILE_PATH_LENGTH] = {0};
	size_t path_len = elix_cstring_length(path, 0);

	current_directory = opendir(path);

	if ( !current_directory ) {
		return directory;
	}

	directory = calloc(1, sizeof(directory));
	while ((entity = readdir(current_directory)) ) {
		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)) {

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) ) {
				directory->count++;
			}
		} else {
			directory->count++;
		}
	}

	
	directory->files = calloc(directory->count, sizeof(elix_path));
	rewinddir(current_directory);

	elix_cstring_copy(path, buffer);
	if ( path[path_len-1] != '/' ) {
		elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, "/", 1);
		path_len++;
	}

	char * filename_buffer = buffer + path_len;
	size_t path_ubffer = ELIX_FILE_PATH_LENGTH - path_len;
	while ( (entity = readdir(current_directory)) ) {
		size_t filename_size = elix_cstring_length(entity->d_name, 0);
		

		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)){

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) ) {
				elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, entity->d_name, filename_size);
				elix_path_update(&(directory->files[index]), buffer);
				index++;
				memset(filename_buffer, 0, filename_size);
			}
		} else {
			elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, entity->d_name, filename_size);
			directory->files[index] = elix_path_create(buffer);
			index++;
			memset(filename_buffer, 0, filename_size);
		}
	}
	closedir(current_directory);
	return directory;
}

bool elix_os_directory_make(const char * path, uint32_t mode, UNUSEDARG bool parent) {
	#if __WIN32__
		if ( ! mkdir(path) ) {
			return true;
		}

		errno_t err;
		_get_errno( &err );
	#else
		int32_t err = mkdir(path, S_IRWXU| S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
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

uint8_t elix_os_directory_is( const char * path){
	#ifdef __WIN32__
	struct _stat64 directory;

	if ( !_stat64( path, &directory ) ) {
		return !!(directory.st_mode & _S_IFDIR);
	}

	errno_t err;
	_get_errno( &err );

	#else
	struct stat64 directory;

	int32_t err = stat64( path, &directory );
	if ( !err ) {
		return S_ISDIR(directory.st_mode);
	}

	#endif
	if ( err == ENOENT ) {
		//LOG_MESSAGE("Path was not found. %s", path);
	} else if ( err == EINVAL  ) {
		LOG_MESSAGE("Invalid parameter; . %s", path);
	}

	return false;
}

void elix_os_command_capture( char * program, char * buffer, size_t buffer_size, char ** env, ...) {
	int err = 0;
	size_t count = 0;
	size_t str_size = 0;
	size_t str_position = 0;

	char * command_string = nullptr;

	va_list passed_arguments, count_arguments;
	va_start(passed_arguments, env);
	va_copy(count_arguments, passed_arguments);
	while ( true ) {
		char * carg = va_arg(count_arguments, char*);
		if (!carg) break;
		str_size += elix_cstring_length(carg, 1);
		count++;
	}
	va_end(count_arguments);

	str_size += elix_cstring_length(program, 1) + count;

	if ( str_size > 0x1FF ) {
		LOG_INFO("Command lenght is to much: %s", program);
		return;
	}

	command_string = calloc(str_size, sizeof(char));

	str_position = elix_cstring_append(command_string, str_size, program, elix_cstring_length(program, 1) );

	if ( count ) {
		for ( size_t i = 1; i <= count; i++) {
			if ( str_position < str_size ) {
				command_string[str_position-1] = ' ';
			}
			char * current_arg = va_arg(passed_arguments, char*);
			str_position = elix_cstring_append(command_string, str_size, current_arg, elix_cstring_length(current_arg, 1) );
		}
	}


	FILE * command_pipe = popen(command_string, "r");
	if ( !command_pipe ) {
		LOG_INFO("Error running command: %s", program);
	} else {
		size_t q = fread(buffer, buffer_size, 1, command_pipe);
		pclose(command_pipe);
	}

	va_end(passed_arguments);

	free_if(command_string);

}

void elix_os_command( char * program, char ** env, ...) {
//
	int err = 0;
	size_t count = 0;
	size_t str_size = 0;
	char ** arguments = nullptr;

	va_list passed_arguments, count_arguments;
	va_start(passed_arguments, env);
	va_copy(count_arguments, passed_arguments);
	while ( true ) {
		char * carg = va_arg(count_arguments, char*);
		if (!carg) break;
		count++;
	}
	va_end(count_arguments);

	if ( count ) {
		arguments = calloc((count + 2) , sizeof(char *));
		arguments[0] = program;
		for ( size_t i = 1; i <= count; i++) {
			arguments[i]  = va_arg(passed_arguments, char*);
		}
	}

	#ifdef __WIN32__
	err = _spawnvp( _P_WAIT, arguments[0], (const char *const *) arguments );
	#else
	id_t pid;
	err = posix_spawnp(&pid, arguments[0], nullptr, nullptr, arguments, nullptr );
	#endif
	
	if ( err != 0 ) {
		LOG_INFO("Error running command: %s", program);
	}

	va_end(passed_arguments);

	free_if(arguments);

}

// < OS

/*** Genscript  ***/

typedef struct {
	uint32_t id;
	char note[16];
	char text[1024];
} PlatformConfig;

typedef struct {
	char stage[16];
	char oext[16];
	char content[256];
} ExtensionOutput;

typedef struct {
	char name[16];
	char flag[512];
	char libflag[512];
	char define[256];
} CompilerConfigOption;

#define PACK_ID(A, B, C, D) (A | B << 8 | C << 16 | D << 24 )
#define UNPACK_ID(ID) { ID & 0xFF, (ID & 0xFF00) >> 8, (ID & 0xFF0000) >> 16, (ID & 0xFF000000) >> 24, 0 }

typedef struct {
	uint32_t id; // Use PACK_ID macro
	char filename[64];
	char content[1024];
} EditorFile;

/* Genscript Default Settings */

static char config_arch_text[512] = "[defines]\n\
PLATFORM_BITS=$bits\n\
[options]\n\
compiler=$tripletgcc\n\
linker=$tripletgcc\n\
static_linker=ar -rc\n\
";

static char config_default_text[2048] = "[options]\n\
compiler=$tripletgcc\n\
linker=$tripletgcc\n\
static_linker=ar -rc\n\
program_suffix=\n\
shared_suffix=.so\n\
static_suffix=.a\n\
cppStandard=c++14\n\
cStandard=c11\n\
[includes]\n\
./include/\n\
[options-windows]\n\
program_suffix=.exe\n\
shared_suffix=.dll\n\
[options-3ds]\n\
finaliser=3dsxtool\n\
program_suffix=.elf\n\
finalise_suffix=.3dsx\n\
[commands]\n\
compile_cpp=${compiler} ${compiler_includes} ${compller_defines} ${compiler_cpp_flags} ${compiler_flags} -o $out -c $in\n\
compile_c=${compiler} ${compiler_includes} ${compller_defines} ${compiler_c_flags} ${compiler_flags} -o $out -c $in\n\
link_shared=${linker} -shared ${compiler_lib_flags} $in -o ${binary_prefix}$out  ${compiler_lib}\n\
link_static=${static_linker} ${object_dir}/$out $in \n\
link=${linker} ${compiler_lib_flags} $in -o ${binary_prefix}$out ${compiler_lib} \n\
finalise=${finaliser} ${finalise_flags} $in -o ${binary_prefix}$out\n\
build_resources=echo\n\
clean=rm -rf ${object_dir}\n\
";

static CompilerConfigOption compiler_debug_options[8] = {
	{"gcc", "-ggdb3", "-ggdb3 ", "_DEBUG"},
	{"gcc-full", "-ggdb3 -fsanitize=address,undefined -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fasynchronous-unwind-tables", 
	"-ggdb3 -fsanitize=address,undefined -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fasynchronous-unwind-tables", "_DEBUG"},
	{"clang", "-ggdb3", "-ggdb3 ", "_DEBUG"},
};

static ExtensionOutput ninja_extension_output[32] = {
	{"link_stage", "", "build %s${binary_suffix}${program_suffix}: link %s\n"},
	{"lk_shared_stage", "", "build %s${binary_suffix}${shared_suffix}: link_shared %s\n"},
	{"lk_static_stage", "", "build %s${binary_suffix}${static_suffix}: link_static %s\nbuild ${object_dir}/%s${binary_suffix}${static_suffix}: phony %s${binary_suffix}${static_suffix}\n"},
	{"resource_stage", "", "build ${object_dir}/resources: build_resources %s\n"},
	{"final_stage", "", "build %s${binary_suffix}${finalise_suffix}: finalise %s${binary_suffix}${program_suffix}\n"},
	{"cpp", "o", "build ${object_dir}%s.%s: compile_cpp %s.cpp\n"},
	{"cc", "o", "build ${object_dir}%s.%s: compile_cpp %s.cc\n"}, // .cc is used for C-style C++ code
	{"c", "o", "build ${object_dir}%s.%s: compile_c %s.c\n"},
	{ "", "", ""}
};

static ExtensionOutput bash_extension_output[32] = {
	{"link_stage", "", "\t${linker} ${compiler_lib_flags} -o ${binary_prefix}%s${binary_suffix}${program_suffix} %s ${compiler_lib}\n"},
	{"lk_shared_stage", "", "\t${linker} -shared ${compiler_lib_flags}  -o ${binary_prefix}%s${binary_suffix}${shared_suffix} %s ${compiler_lib}\n"},
	{"lk_static_stage", "", "${static_linker} ${object_dir}$out $in \n"},
	{"resource_stage", "", "TODO\n"},
	{"final_stage", "", "TODO\n"},
	{"cpp", "o", "\t${compiler} ${compiler_includes} ${compller_defines} ${compiler_cpp_flags} ${compiler_flags} -o ${object_dir}%s.%s -c %s.cpp\n"},
	{"cc", "o", "\t${compiler} ${compiler_includes} ${compller_defines} ${compiler_cpp_flags} ${compiler_flags} -o ${object_dir}%s.%s -c %s.cc\n"}, // .cc is used for C-style C++ code
	{"c", "o", "\t${compiler} ${compiler_includes} ${compller_defines} ${compiler_c_flags} ${compiler_flags} -o ${object_dir}%s.%s -c %s.c\n"},
	{ "", "", ""}
};
static ExtensionOutput batch_extension_output[32] = {
	{"link_stage", "", "\t%%linker%% %%compiler_lib_flags%% -o %%binary_prefix%%%s%%binary_suffix%%%%program_suffix%% %s %%compiler_lib%%\n"},
	{"lk_shared_stage", "", "\t%%linker%% -shared %%compiler_lib_flags%% -o %%binary_prefix%%%s%%binary_suffix%%%%shared_suffix%% %s %%compiler_lib%%\n"},
	{"lk_static_stage", "", "TODO\n"},
	{"resource_stage", "", "TODO\n"},
	{"final_stage", "", "TODO\n"},
	{"cpp", "o", "\t%%compiler%% %%compiler_includes%% %%compller_defines%% %%compiler_flags%% -o %%object_dir%%%s.%s -c %s.cpp\n"},
	{"cc", "o", "\t%%compiler%% %%compiler_includes%% %%compller_defines%% %%compiler_flags%% -o %%object_dir%%%s.%s -c %s.cc\n"}, // .cc is used for C-style C++ code
	{"c", "o", "\t%%compiler%% %%compiler_includes%% %%compller_defines%% %%compiler_flags%% -o %%object_dir%%%s.%s -c %s.c\n"},
	{ "", "", ""}
};

static PlatformConfig config_platform_text[] = {
	{ 0, "Default", "[libs-cpp]\nstdc++\n[libs]\nm\n[lib_flags]\n[includes]\n[flags]\n[defines]\nPLATFORM_$PLATFORM\n[linker_flags]\n-Wl,-rpath -Wl,\\$$ORIGIN/lib\n"},
	{ 0x6986d1e1, "3DS", "[libs]\nstdc++\nm\nctru\ncitro3d\n[lib_flags]\n-std=c++11\n-march=armv6k\n-mtune=mpcore\n-mfloat-abi=hard\n-mtp=soft\n-specs=3dsx.specs\n-L\"/opt/devkitpro/devkitARM/lib\"\n-L\"/opt/devkitpro/libctru/lib\"\n[includes]\n\"/opt/devkitpro/devkitARM/arm-none-eabi/include\"\n\"/opt/devkitpro/libctru/include\"\n[flags]\n-std=c++11\n-march=armv6k\n-mtune=mpcore\n-mfloat-abi=hard\n[defines]\nPLATFORM_3DS\nPLATFORM_BITS=32\nARM11\n[commands]\nfinalise=${finaliser} $in  $out ${finalise_flags}\nbuild_smdh=smdhtool --create -o $out\nbuild_pica=./genscript.exe -h=$out -i=$in.temp -c=\"picasso -o $in.temp $in\"\n[extension]\npica=h;build res/%s.h: build_pica %s.pica\n"}
};

/* */
static EditorFile editor_files[] = {
	{PACK_ID('V','S','C','o'), ".vscode/", ""},
	{PACK_ID('V','S','C','o'), ".vscode/tasks.json", "{\n\t\"version\": \"2.0.0\",\n\t\"tasks\":[\n\t\t{\n\t\t\t\"type\": \"shell\",\n\t\t\t\"label\": \"Build All\",\n\t\t\t\"command\": \"ninja\",\n\t\t\t\"problemMatcher\": [\n\t\t\t\t\"$gcc\"\n\t\t\t],\n\t\t\t\"group\": {\n\t\t\t\t\"kind\": \"build\",\n\t\t\t\t\"isDefault\": true\n\t\t\t}\n\t\t},\n\t\t{\n\t\t\t\"type\": \"shell\",\n\t\t\t\"label\": \"Reconfig\",\n\t\t\t\"command\": \"./gsb.exe\",\n\t\t\t\"group\": {\n\t\t\t\t\"kind\": \"build\",\n\t\t\t\t\"isDefault\": false\n\t\t\t}\n\t\t}\n\t\t]\n\t\n}"},
	{PACK_ID('V','S','C','o'), ".vscode/settings.json", "{\n\t\"triggerTaskOnSave.on\": true,\n\t\"triggerTaskOnSave.tasks\": {\n\t\t\"Build All\": [\n\t\t\t\"*.c\",\n\t\t\t\"*.cc\",\n\t\t\t\"*.h\",\n\t\t\t\"*.cpp\",\n\t\t\t\"*.hpp\",\n\t\t],\n\t\t\"Reconfig\": [\n\t\t\t\"*.txt\",\n\t\t],\n\t},\n}"},
	{PACK_ID('V','S','C','o'), ".vscode/launch.json", "{\n\t\"version\": \"0.2.0\",\n\t\"configurations\": [\n\t\t{\n\t\t\t\"name\":\"Debug\",\n\t\t\t\"type\":\"cppdbg\",\n\t\t\t\"request\": \"launch\",\n\t\t\t\"cwd\": \"${workspaceRoot}\",\n\t\t\t\"program\": \"${workspaceRoot}/bin/bookish-debug-linux-x86_64\"\n\t\t}\n\t]\n}\n" },



};

/* genscript enum & struct */
typedef struct {
	char os[16];
	char arch[8];
	char compiler[8];
	char mode[8];
	char bits[4];
	char triplet[32];
	uint32_t os_hash;
	uint32_t build_system;
} CompilerInfo;

typedef enum {
	SM_NONE, SM_FILE, SM_DEFINES, SM_FLAGS, SM_LIBS, SM_LIB_FLAGS, SM_FINAL_FLAGS, SM_INCLUDES, SM_MODULES, SM_OTHER
} scan_mode;

typedef enum {
	PM_HELP, PM_INFO, PM_NEWPROJECT, PM_NEWPLATFORM, PM_GEN, PM_UPDATEMODULE, PM_EDITORCONFIG, PM_B2H, PM_REBUILDSELF, PM_SWITCH, PM_INTERACTIVE, PM_AUTO
} program_mode;

typedef enum {
	JCO_DEFINES, JCO_FLAGS, JCO_LIBS, JCO_LIBS_FLAGS, JCO_FINAL_FLAGS, JCO_INCLUDE
} join_config_options;

typedef enum {
	LT_PROGRAM, LT_SHARED, LT_STATIC, LT_SKIP
} linking_type;

typedef struct {
	uint8_t current;
	uint32_t hash[64];
	char key[64][256];
	char value[64][256];
} ConfigMap;

typedef struct {
	uint8_t current;
	char value[255][256];
} ConfigList;

typedef struct {
	ConfigList files;
	ConfigList resources;
	ConfigList defines;
	ConfigList flags;
	ConfigList libs;
	ConfigList lib_flags;
	ConfigList final_flags;
	ConfigList includes;
	ConfigList modules;
	ConfigMap options;
	ConfigMap commands;
} CurrentConfiguration;

typedef struct {
	char formatting[32];
	ConfigList * option;
	char * skip_formatting_prefix;
	char * buffer;
} JoinConfigList;


size_t find_configmap(ConfigMap * map, const char * key ) {
	if (  map->current > 0 && map->current < 64 ) {
		size_t length = elix_cstring_length(key, 0);
		uint32_t hash = elix_hash(key, length);
		for (size_t i = 0; i < map->current; i++) {
			if ( map->hash[i] == hash ) {
				return i;
			}
		}
	}
	return SIZE_MAX;
}

void lookup_configmap(ConfigMap * map, uint32_t hash, size_t * position) {
	if (  map->current > 0 && map->current < 64 ) {
		for (size_t i = 0; i < map->current; i++) {
			if ( map->hash[i] == hash ) {
				*position = i;
				return;
			}
		}
	}
}

char * get_configmap(ConfigMap * map, const char * key) {
	size_t index = find_configmap(map, key);
	if ( index != SIZE_MAX && map->current < 64 ) {
		return map->value[index];
	}
	return nullptr;
}

void pushset_configmap(ConfigMap * map, const char * key, const char * value, uint8_t overwrite) {
	if (  map->current < 64 ) {
		size_t position = map->current;
		size_t length = elix_cstring_length(key, 0);
		size_t vlength = elix_cstring_length(value, 0);
		lookup_configmap(map, elix_hash(key, length), &position);
		if ( overwrite ) {
			map->value[position][0] = 0;
		}
		elix_cstring_append(map->value[position], 256, value, vlength);

		if ( position == map->current ) {
			map->hash[position] = elix_hash(key, length);
			elix_cstring_append(map->key[position], 256, key, length);
			map->current++;
		}

	}
}

void push_configmap(ConfigMap * map, const char * data, uint8_t overwrite) {
	if (  map->current < 64 ) {
		size_t position = map->current;
		size_t length = elix_cstring_length(data, 0);
		size_t index = elix_cstring_find_of(data, "=", 0);
		if ( !index || index == SIZE_MAX || index > length) {
			index = length;
		} else {
			lookup_configmap(map, elix_hash(data, index), &position);
			if ( overwrite ) {
				map->value[position][0] = 0;
			}
			elix_cstring_append(map->value[position], 256, data + index + 1, length - index);
		}

		//New item
		if ( position == map->current ) {
			map->hash[position] = elix_hash(data, index);
			elix_cstring_append(map->key[position], 256, data, index);
			map->current++;
		}
	}
}

void pushunique_configlist(ConfigList * list, const char * data) {
	if ( list->current < 254 ) {
		size_t length = elix_cstring_length(data, 0);
		uint8_t lookup_count = 0;
		while (lookup_count < list->current ) {
			if ( elix_cstring_match(data, list->value[lookup_count], 256) )
				return;
			lookup_count++;
		}
		elix_cstring_append(list->value[list->current], 256, data, length);
		list->current++;
	}
}

void push_configlist(ConfigList * list, const char * data) {
	if (  list->current < 254 ) {
		size_t length = elix_cstring_length(data, 0);
		elix_cstring_append(list->value[list->current], 256, data, length);
		list->current++;
	}
}

size_t is_in_configlist(ConfigList * list, const char * data) {
	size_t founded = 0;
	if (  list->current > 0 && list->current < 254 ) {
		for (size_t i = 0; i < list->current; i++) {
			founded += elix_cstring_has_prefix(list->value[i], data);
		}
	}
	return founded;
}


void push_extensionoutput(ExtensionOutput * list, const char * stage, const char * oext, const char * content) {
	uint8_t index = 5;
	//Find first empty index
	while (  index < 32 ) {
		if ( !list[index].stage[0] ) {
			elix_cstring_copy(stage, list[index].stage);
			elix_cstring_copy(oext, list[index].oext);
			elix_cstring_copy(content, list[index].content);
			return;
		}
		index++;
	}
}

void pushstring_extensionoutput(ExtensionOutput * list,  const char * data) {
	uint8_t list_index = 5;
	//Find first empty index
	size_t length = elix_cstring_length(data, 0);
	size_t index = elix_cstring_find_of(data, "=", 0);
	size_t index2 = elix_cstring_find_of(data, ";", index);
	if ( !index || index == SIZE_MAX || index > length) {
		return;
	}
	if ( !index2 || index2 == SIZE_MAX || index2 > length) {
		index2 = index;
	}

	char * stage = elix_cstring_from(data, "unknown", index+1);
	char * oext = elix_cstring_from(data + index + 1, "*", index2 - index); 
	char * content  = elix_cstring_from(data + index2 + 1, "ERROR", length - index2);


	while (  list_index < 32 ) {
		if ( !list[list_index].stage[0] ) {
			elix_cstring_copy(stage, list[list_index].stage);
			elix_cstring_copy(oext, list[list_index].oext);
			elix_cstring_copy(content, list[list_index].content);
			return;
		}
		list_index++;
	}
}

void find_plaform_details(CompilerInfo * info) {
	uint32_t os = elix_hash(info->os, elix_cstring_length(info->os, 0));
	switch (os)
	{
	case 0x6986d1e1: //3DS
		elix_cstring_copy("arm-none-eabi-", info->triplet);
		elix_cstring_copy("arm", info->arch);
		elix_cstring_copy("32", info->bits);
		break;
	
	default:
		break;
	}

}


CompilerInfo check_preprocessor() {
	CompilerInfo info;
	memset(&info, 0, sizeof(CompilerInfo));
	#if defined(__MINGW64__)
		#define PLATFORM "windows"
		#define PLATFORM_ARCH "x86_64"
		#define PLATFORM_COMPILER "gcc"
		#define PLATFORM_BITS 64
	#elif defined(__MINGW32__)
		#define PLATFORM "windows"
		#define PLATFORM_ARCH "x86"
		#define PLATFORM_COMPILER "gcc"
		#define PLATFORM_BITS 32
	#elif defined(__linux__)
		#define PLATFORM "linux"
	#elif defined(_WIN64)
		#define PLATFORM "windows"
		#define PLATFORM_ARCH "x86_64"
		#define PLATFORM_BITS 64
	#elif defined(_WIN32)
		#define PLATFORM "windows"
		#define PLATFORM_ARCH "x86"
		#define PLATFORM_BITS 32
	#elif defined(BSD)
		#define PLATFORM "bsd"
	#elif defined(__BEOS__)
		#define PLATFORM "haiku"
	#else
		#define PLATFORM "unknown"
	#endif

	#if !defined(PLATFORM_ARCH)
		#if defined(__x86_64__)
			#define PLATFORM_ARCH "x86_64"
			#define PLATFORM_BITS 64
		#elif defined(_M_AMD64)
			#define PLATFORM_ARCH "x86_64"
			#define PLATFORM_BITS 64
		#elif defined(__i686__)
			#define PLATFORM_ARCH "x86"
			#define PLATFORM_BITS 32
		#elif defined(__aarch64__)
			#define PLATFORM_ARCH "ARM64"
			#define PLATFORM_BITS 64
		#elif defined(__arm__)
			#define PLATFORM_ARCH "ARM"
			#define PLATFORM_BITS 32
		#elif defined(__mips__)
			#define PLATFORM_ARCH "MIPS"
			#define PLATFORM_BITS 32
		#elif defined(__sh__)
			#define PLATFORM_ARCH "SH"
			#define PLATFORM_BITS 32
		#else
			#define PLATFORM_ARCH "unknown"
		#endif
	#endif
	#if !defined(PLATFORM_BITS)
		#if defined(__LP64__)
			#define PLATFORM_BITS 64
		#elif defined(__LP32__)
			#define PLATFORM_BITS 32
		#else
			#define PLATFORM_BITS 32
		#endif
	#endif

	#if !defined(PLATFORM_COMPILER)
		#if defined(__GNUC__)
			#define PLATFORM_COMPILER "gcc"
		#elif defined(__clang__)
			#define PLATFORM_COMPILER "clang"
		#elif defined(__llvm__)
			#define PLATFORM_COMPILER "llvm"
		#elif defined(_MSC_VER)
			#define PLATFORM_COMPILER "msvc"
		#else
			#define PLATFORM_COMPILER "unknown"
		#endif
	#endif


	elix_cstring_copy(PLATFORM, info.os);
	elix_cstring_copy(PLATFORM_ARCH, info.arch);
	elix_cstring_copy(PLATFORM_COMPILER, info.compiler);
	elix_cstring_copy(DEFAULT_BUILD_MODE, info.mode);

	snprintf(info.bits, 3, "%d", PLATFORM_BITS);

	info.os_hash = elix_hash(info.os, elix_cstring_length(info.os, 0));

	return info;
}

uint64_t extension_ident( const char * extension ) {
	size_t length = elix_cstring_length(extension, 0);
	uint64_t ident = 0;
	if ( length > 8 ) {
		length = 8;
	}
	for (size_t var = 0; var < length; ++var) {
		ident = (ident << 8) + extension[var];
	}
	return ident;
}

void update_string_from_compilerinfo( char * source_text, size_t source_size, CompilerInfo * target ) {
	elix_cstring_inreplace(source_text, source_size, "$platform", target->os, 1);
	elix_cstring_inreplace(source_text, source_size, "$arch", target->arch, 1);
	elix_cstring_inreplace(source_text, source_size, "$compiler", target->compiler, 1);
	elix_cstring_inreplace(source_text, source_size, "$mode", target->mode, 1);
	elix_cstring_inreplace(source_text, source_size, "$bits", target->bits, 1);
	elix_cstring_inreplace(source_text, source_size, "$triplet", target->triplet, 1);
	elix_cstring_inreplace(source_text, source_size, "$buildmode", target->mode, 1);

	char os[16];
	elix_cstring_copy(target->os, os);
	elix_cstring_transform(os, ELIX_CHAR_UPPER);
	elix_cstring_inreplace(source_text, source_size, "$PLATFORM", os, 1);
}

char *  elix_cstring_formatted( const char * format, ... ) {
	size_t written = 0;
	va_list args;
	char * buffer = nullptr;
	va_start(args, format);
	written = vsnprintf( nullptr, 0, format, args);
	va_end(args);
	return buffer;
}


size_t elix_file_write_formatted( elix_file * file, const char * format, ... ) {
	if ( !file || !file->handle ) {
		return 0;
	}
	size_t written = 0;
	va_list args;
	char buffer[1024];
	va_start(args, format);
	written = vfprintf( FH(file->handle), format, args);
	va_end(args);
	
	return written;
}

size_t elix_file_write_string_from_compilerinfo( elix_file * file, const char * string, CompilerInfo * target ) {
	if ( !file || !file->handle ) {
		return 0;
	}
	char buffer[1024] = {0};
	elix_cstring_copy(string, buffer);
	update_string_from_compilerinfo(buffer, 1024, target);

	return fwrite(buffer, elix_cstring_length(buffer, 0), 1, FH(file->handle));
}

uint8_t scan_filelist( char * file_path, const char * module, ConfigList * files, ConfigList * module_list, ConfigList * link_objects) {

	char * trim_path = file_path;
	char * path_ext = nullptr;

	size_t newl = elix_cstring_find_of(file_path, "/*.", 0);
	size_t extl = elix_cstring_length(file_path, 0);
	if ( newl < ELIX_FILE_PATH_LENGTH ) {
		trim_path = malloc(newl);
		elix_cstring_copy2(file_path, trim_path, newl);

		if ( !elix_cstring_match(file_path+newl+1, "*.*", 4) ) {
			path_ext = malloc(extl-newl);
			elix_cstring_copy2(file_path+newl+2, path_ext, extl-newl-2);
		}
	}	
	
	elix_directory * directory = elix_os_directory_list_files(trim_path, path_ext);
	if ( directory ) {
		for (size_t b = 0; b < directory->count; ++b) {
			if ( !elix_os_directory_is(directory->files[b].uri) ) {
				elix_cstring_copy(directory->files[b].uri, files->value[files->current]);
				files->current++;
			}
		}
		elix_os_directory_list_destroy(&directory);
	}

	if ( trim_path != file_path ) {
		free(trim_path);
	}
	free_if(path_ext);
	return 0;
}

uint8_t parse_filelist( const char * module, ConfigList * files, ConfigList * module_list, ConfigList * link_objects) {
	uint8_t link = LT_PROGRAM;
	char filename[512] = "./config/modules/$module.txt";
	uint8_t counter = 0;
	elix_cstring_inreplace(filename, 512, "$module", module,0);

	elix_file file = {0};
	elix_file_open(&file, filename, EFF_FILE_READ);
	while (!elix_file_at_end(&file)) {
		char data[256] = {0};
		elix_file_read_line(&file, data, 256);
		elix_cstring_trim(data);
		//LOG_INFO("Module: '%s' requires: '%s'", module, data);
		if ( elix_cstring_has_prefix(data, "#") ) {
			//LOG_INFO("Note: '%s' requires: '%s' module", module, data + 1);
			pushunique_configlist(module_list, data + 1);

			elix_cstring_copy("", link_objects->value[link_objects->current]);
			
			elix_cstring_append(link_objects->value[link_objects->current], 255, "${object_dir}", 14);
			elix_cstring_append(link_objects->value[link_objects->current], 255, data + 1, elix_cstring_length(data + 1, 0));
			elix_cstring_append(link_objects->value[link_objects->current], 255, "${binary_suffix}${static_suffix} ", 34);
			
			//LOG_INFO("%d %s", link_objects->current, link_objects->value[link_objects->current]);
			link_objects->current++;
		} else if ( elix_cstring_has_prefix(data, "//") ) {
			
		} else if ( elix_cstring_has_prefix(data, "$shared") ) {
			link = LT_SHARED;
		} else if ( elix_cstring_has_prefix(data, "$static") ) {
			link = LT_STATIC;
		} else if ( elix_cstring_has_prefix(data, "./") ) {
			if ( elix_cstring_find_of(data, "/*.", 1) != SIZE_MAX) {
				scan_filelist(data, module, files, module_list, link_objects);
			} else {
				elix_cstring_copy(data + 2, files->value[files->current]);
			}
			counter++;
		} else if ( data[0] ) {
			if ( elix_cstring_find_of(data, "/*.", 0) != SIZE_MAX ) {
				scan_filelist(data, module, files, module_list, link_objects);
			} else {
				elix_cstring_copy(data, files->value[files->current]);
			}
			
			counter++;
		}
		files->current++;
	}

	if ( counter == 0) {
		link = LT_SKIP;
	}
	
	return link;
}


void parse_package_linux( char data[256], CurrentConfiguration * options, CompilerInfo * compiler) {
	char arguments[][16] = {
		"--cflags",
		"--libs"
	};

	ConfigList * config_index[] = {
		&options->flags,
		&options->libs
	};

	char output[512] = {};
	char package_name[16] = {};
	elix_cstring_copy2(data, package_name, 15);

	for ( uint8_t i = 0; i < 2;  i++) {
		elix_os_command_capture( "pkg-config", output, 512, nullptr /*env*/, arguments[i], package_name, nullptr);
		elix_cstring_trim(output);

		char ** split_arguments = elix_cstring_split( output, ' ', 0);
		if ( split_arguments ) {
			size_t in = 0;
			while (split_arguments[in]) {
				LOG_INFO("Package %s, %s returned %s", package_name, arguments[i], split_arguments[in]);
				push_configlist(config_index[i], split_arguments[in]);
				in++;
			}
			free(split_arguments);
		} else {
			LOG_INFO("Package %s, %s returned %s", package_name, arguments[i], output);
			push_configlist(config_index[i], output);
		}

		memset(output, 0, 512);
	}
}


void parse_package( char data[256], CurrentConfiguration * options, CompilerInfo * compiler) {

	switch (compiler->os_hash)
	{
		case 0xd03ded56: //Linux
			parse_package_linux(data, options,compiler);
		break;

		default:
		break;
	}

}

uint32_t parse_txtcfg( const char * filename, CurrentConfiguration * options, CompilerInfo * compiler) {
	LOG_INFO("\tReading %s", filename);
	elix_file file = {0};
	ConfigList * write_options = nullptr;
	ConfigMap * write_map = nullptr;
	uint8_t write_mode = 0;
	char platform_option[64] = {0};
	char commands_option[64] = {0};
	char package_option[64] = {0};
	snprintf(platform_option, 64, "[options-%s]", compiler->os);
	snprintf(commands_option, 64, "[commands-%s]", compiler->os);
	snprintf(package_option, 64, "[pkg-%s]", compiler->os);
	if ( elix_file_open(&file, filename, EFF_FILE_READ) != 0 )
		return 0;
	while (!elix_file_at_end(&file)) {
		char data[256] = {0};
		elix_file_read_line(&file, data, 256);

		if ( data[0] == '[') {
			if ( elix_cstring_has_prefix(data,"[defines]") ) {
				write_mode = 1;
				write_options = &options->defines;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[extension]") ) {
				write_mode = 3;
				write_options = nullptr;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[files]") ) {
				write_mode = 1;
				write_options = &options->files;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[resources]") ) {
				write_mode = 1;
				write_options = &options->resources;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[lib_flags]") ) {
				write_mode = 1;
				write_options = &options->lib_flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[compiler_flags]") ) {
				write_mode = 1;
				write_options = &options->flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[final_flags]") ) {
				write_mode = 1;
				write_options = &options->final_flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[flags]") ) {
				write_mode = 1;
				write_options = &options->flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[includes]") ) {
				write_mode = 1;
				write_options = &options->includes;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[libs]") ) {
				write_mode = 1;
				write_options = &options->libs;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[options]") ) {
				write_mode = 2;
				write_options = nullptr;
				write_map = &options->options;
			} else if ( elix_cstring_has_prefix(data,platform_option) ) {
				write_mode = 2;
				write_options = nullptr;
				write_map = &options->options;
			} else if ( elix_cstring_has_prefix(data,"[commands]") ) {
				write_mode = 2;
				write_options = nullptr;
				write_map = &options->commands;
			} else if ( elix_cstring_has_prefix(data,commands_option) ) {
				write_mode = 2;
				write_options = nullptr;
				write_map = &options->commands;
			} else if ( elix_cstring_has_prefix(data,package_option) ) {
				write_mode = 16;
				write_options = nullptr;
				write_map = nullptr;
			} else {
				write_mode = 0;
				write_options = nullptr;
				write_map = nullptr;
			}
		} else if ( data[0] == '#') {
		} else if ( data[0] < 32 ) {

		} else {
			elix_cstring_trim(data);
			update_string_from_compilerinfo(data, 256, compiler);
			if ( write_options ) {
				push_configlist(write_options, data);
			} else if (write_map) {
				push_configmap(write_map, data, 1);
			} else if (write_mode == 3) {
				pushstring_extensionoutput(ninja_extension_output, data);
			} else if (write_mode == 16) {
				elix_cstring_transform(data, ELIX_CHAR_LOWER);
				LOG_INFO("Package Lookup: %s", data);
				parse_package(data, options, compiler);
			} else {
				//LOG_INFO("Config option skipped: %s", data);
			}
		}
	}
	elix_file_close(&file);
	return 1;
}

size_t join_config_option( JoinConfigList * item) {
	if ( item->buffer == nullptr ) {
		item->buffer = malloc(1024);
	}

	memset(item->buffer, 0, 1024);
	
	for (size_t i = 0; i < item->option->current; i++) {
		char tempbuffer[128] = {0};
		if( item->skip_formatting_prefix && elix_cstring_has_prefix( item->option->value[i], item->skip_formatting_prefix) ) {
			size_t q = snprintf(tempbuffer, 128, "%s ", item->option->value[i]);
			elix_cstring_append(item->buffer, 1024, tempbuffer, q);
		} else {
			size_t q = snprintf(tempbuffer, 128, item->formatting, item->option->value[i]);
			elix_cstring_append(item->buffer, 1024, tempbuffer, q);
		}
	}

	return elix_cstring_length(item->buffer, 0);
}

uint32_t fg_build_ninja(CompilerInfo * target, CurrentConfiguration * options, char * filename ){
	LOG_INFO("Building: %s", filename);
	
	JoinConfigList ini_list[] = {
		{ "-D%s ", &options->defines, "-D", nullptr},
		{ "%s ", &options->flags, nullptr, nullptr},
		{ "-l%s ", &options->libs, "-l", nullptr},
		{ "%s ", &options->lib_flags, nullptr, nullptr},
		{ "%s ", &options->final_flags, nullptr, nullptr},
		{ "-I%s ", &options->includes, "-I", nullptr},
	};
	char platform_file[128] = "./config/$platform-common.txt";
	char arch_file[128] = "./config/$platform-$arch.txt";
	char compiler_common_file[128] = "./config/$platform-$arch-$compiler-$buildmode.txt";
	char compiler_file[128] = "./config/$platform-$arch-$compiler.txt";

	char * program_name = get_configmap(&options->options, "name");

	update_string_from_compilerinfo(platform_file, 128, target);
	update_string_from_compilerinfo(arch_file, 128, target);
	update_string_from_compilerinfo(compiler_file, 128, target);
	update_string_from_compilerinfo(compiler_common_file, 128, target);

	//Read Default
	parse_txtcfg("./config/default.txt", options, target);
	parse_txtcfg("./config/project.txt", options, target);
	parse_txtcfg(platform_file, options, target);
	parse_txtcfg(arch_file, options, target);
	parse_txtcfg(compiler_common_file, options, target);
	parse_txtcfg(compiler_file, options, target);

	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++) {
		join_config_option(&ini_list[i]);
	}

	///NOTE: Workaround for -static-libstdc++ arg in libs
	if ( is_in_configlist(&options->lib_flags, "-static-libstdc++"))
	{
		char * buffer = get_configmap(&options->options, "linker");
		if ( elix_cstring_has_suffix(buffer, "gcc") || elix_cstring_has_suffix(buffer, "gcc.exe")) {
			LOG_INFO("\tUsing -static-libstdc++ requires using g++ instead of gcc as linker.");
			elix_cstring_inreplace(buffer, 128, "gcc", "g++", 1);
		}
	}


	//Write
	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_TRUNCATE);

	elix_file_write_string(&file, "ninja_required_version = 1.3\n", 0);
	elix_file_write_string_from_compilerinfo(&file, "builddir=compile/$platform-$arch/log\n", target);

	elix_file_write_formatted(&file, "compiler_lib = %s\n", ini_list[JCO_LIBS].buffer);
	elix_file_write_formatted(&file, "compiler_lib_flags = %s\n", ini_list[JCO_LIBS_FLAGS].buffer);
	elix_file_write_formatted(&file, "compiler_flags = %s\n", ini_list[JCO_FLAGS].buffer);
	elix_file_write_formatted(&file, "compiler_includes = %s\n", ini_list[JCO_INCLUDE].buffer);
	elix_file_write_formatted(&file, "compller_defines = %s\n", ini_list[JCO_DEFINES].buffer);
	elix_file_write_string_from_compilerinfo(&file, "compiler_mode = $mode\n", target);

	if ( find_configmap(&options->options, "cppStandard") != SIZE_MAX ) {
		elix_file_write_formatted(&file, "compiler_cpp_flags = -std=%s\n", get_configmap(&options->options, "cppStandard"));
	}
	if ( find_configmap(&options->options, "cStandard") != SIZE_MAX ) {
		elix_file_write_formatted(&file, "compiler_c_flags = -std=%s\n", get_configmap(&options->options, "cStandard"));
	}

	if ( find_configmap(&options->options, "program_executable") != SIZE_MAX ) {
		elix_file_write_formatted(&file, "program_executable = %s\n", get_configmap(&options->options, "program_executable"));
	} else {
		elix_file_write_formatted(&file, "program_executable = %s\n", program_name);
	}

	

	elix_file_write_formatted(&file, "finaliser_flags = %s\n", ini_list[JCO_FINAL_FLAGS].buffer);

	elix_file_write_string(&file, "binary_prefix = bin/\n", 0);
	elix_file_write_string_from_compilerinfo(&file, "binary_suffix = -$mode-$platform-$arch\n", target);

	char * required_options[] = {
		"finaliser", "compiler", "linker", "static_linker",
		"program_suffix", "finalise_suffix",
		"shared_suffix", "static_suffix"
	};

	for (size_t var = 0; var < ARRAY_SIZE(required_options); var++) {
		char * buffer = get_configmap(&options->options, required_options[var]);
		if ( buffer ) {
			elix_file_write_formatted(&file, "%s = %s\n", required_options[var], buffer);
		}
	}
	
	elix_file_write_string_from_compilerinfo(&file, "object_dir = compile/$platform-$arch/$mode/\n", target);
	elix_file_write_string(&file, "\n", 1);
	//elix_file_write_string(&file,  default_ninja_rules, 0);

	for (size_t i = 0; i < options->commands.current; i++){
		elix_file_write_formatted(&file, "rule %s\n", options->commands.key[i]);
		elix_file_write_formatted(&file, "  command = %s\n", options->commands.value[i]);
		elix_file_write_formatted(&file, "  description = [%s] $in\n\n", options->commands.key[i]);
	}

	elix_file_write_formatted(&file, "rule %s\n", "build_genscript");
	elix_file_write_formatted(&file, "  command = %s\n", "gcc genscript.c -o gsb.exe");
	elix_file_write_formatted(&file, "  description = [%s] $in\n\n", "Build Script Generator");

	elix_file_write_formatted(&file, "rule %s\n", "run_genscript");
	elix_file_write_formatted(&file, "  command = %s\n", "./gsb.exe");
	elix_file_write_formatted(&file, "  description = [%s] $in\n\n", "Run Build Script Generator");

	elix_file_write_formatted(&file, "rule %s\n", "run_program");
	elix_file_write_formatted(&file, "  command = %s\n", "${binary_prefix}${program_executable}${binary_suffix}${program_suffix}");
	elix_file_write_formatted(&file, "  description = [%s] $in\n\n", "Run");


	elix_file_write_string(&file, "\n", 1);
	elix_file_write_string(&file, "build genscript: build_genscript\n", 0);
	elix_file_write_string(&file, "build clean: clean\n", 0);
	elix_file_write_string(&file, "build run: run_program\n", 0);
	elix_file_write_string(&file, "\n", 1);
	elix_file_write_string(&file, "#Files\n", 0);

	

	char objects[6144] = {0};
	char resources[6144] = {0};
	char defaults[256] = {0};

	if ( options->resources.current ) {
		for (size_t i = 0; i < options->resources.current; i++){
			for (size_t var = 5; var < ARRAY_SIZE(ninja_extension_output); var ++) {
					if ( elix_cstring_has_suffix(options->resources.value[i], ninja_extension_output[var].stage) ) {
						size_t leng = elix_cstring_length(options->resources.value[i], 0);
						size_t basename_leng = leng;
						char * basename = nullptr;
						for (size_t split = 0; split < leng; split++) {
							if ( options->resources.value[i][split] == '.') {
								basename_leng = split +1;
								break;
							}
						}
						basename = elix_cstring_from(options->resources.value[i], "NUL", basename_leng);
						elix_file_write_formatted(&file, ninja_extension_output[var].content, basename, options->resources.value[i]);
						elix_file_write_formatted(&file, "\n");

						char format_buffer[128] = {0};
						size_t q = snprintf(format_buffer, 128, "%s%s", basename, ninja_extension_output[var].oext);
						elix_cstring_append(resources, 6144, format_buffer, q);
						free(basename);
						break;
					}
			}

		}
		elix_file_write_formatted(&file, ninja_extension_output[3].content, resources);
		elix_cstring_append(defaults, 255, "${object_dir}/resources ", 24);
	}

	//parse_filelist("base", &options->files, &options->modules);
	for (size_t j = 0; j < options->modules.current; j++) {
		size_t i = options->files.current;
		ConfigList link_objects = {0};
		uint8_t linking_method = parse_filelist(options->modules.value[j], &options->files, &options->modules, &link_objects);
	
		char * current_name = j == 0 ? program_name : options->modules.value[j];
		elix_file_write_formatted(&file, "#Module: %s [%x]\n", options->modules.value[j], linking_method);
		for (; i < options->files.current; i++){
			for (size_t var = 5; var < ARRAY_SIZE(ninja_extension_output); var ++) {
				if ( !ninja_extension_output[var].stage[0] ) {
					break;
				}
				if ( elix_cstring_has_suffix(options->files.value[i], ninja_extension_output[var].stage) ) {
					size_t leng = elix_cstring_length(options->files.value[i], 0);
					for (size_t split = leng; split > 0; split--) {
						if ( options->files.value[i][split] == '.') {
							options->files.value[i][split] = 0;
							leng = split;
							break;
						}
					}
					
					char * no_src_dir = options->files.value[i];
					size_t ifrst_dir_index = elix_cstring_find_of(options->files.value[i], "/", 0);
					if ( ifrst_dir_index < 64 ) {
						no_src_dir  = options->files.value[i] + ifrst_dir_index;
					}

					elix_file_write_formatted(&file, ninja_extension_output[var].content, no_src_dir, ninja_extension_output[var].oext, options->files.value[i]);
					char format_buffer[128] = {0};
					
					size_t q = snprintf(format_buffer, 128, "${object_dir}%s.%s ", no_src_dir, ninja_extension_output[var].oext);
					
					elix_cstring_append(objects, 6144, format_buffer, q);

					break;
				}
			}
		}
		elix_file_write_string(&file, "\n", 1);
		switch ( linking_method ) {
			case LT_STATIC:
				LOG_INFO("\tStatic Lib: %s", current_name);
				elix_file_write_formatted(&file, ninja_extension_output[2].content, current_name, objects,current_name,current_name);
		
				elix_cstring_append(defaults, 255, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 255, "${binary_suffix}${static_suffix} ", 34);

				break;
			case LT_SHARED:
				LOG_INFO("\tShared Lib: %s", current_name);
				elix_file_write_formatted(&file, ninja_extension_output[1].content, current_name, objects);

				elix_cstring_append(defaults, 255, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 255, "${binary_suffix}${shared_suffix} ", 34);

				break;
			case LT_SKIP:
				LOG_INFO("\tModule Skipped: %s", current_name);
				break;
			default:
				LOG_INFO("\tProgram: %s", current_name);
				for (size_t i = 0; i < link_objects.current; i++){
					elix_cstring_append(objects, 6144, link_objects.value[i], elix_cstring_length(link_objects.value[i],0));
				}

				elix_file_write_formatted(&file, ninja_extension_output[0].content, current_name, objects);
				
				elix_cstring_append(defaults, 255, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 255, "${binary_suffix}${program_suffix} ", 35);
				break;
		}
		elix_file_write_string(&file, "\n", 1);
		objects[0] = 0;
	}

	elix_file_write_string(&file, "\n", 1);
	
	if ( find_configmap(&options->options, "finaliser") != SIZE_MAX ) {
		elix_file_write_formatted(&file, "#Finaliser Step\n");
		elix_file_write_formatted(&file, ninja_extension_output[2].content, program_name, program_name);
	} else {

	}

	elix_file_write_formatted(&file, "####################################\n");
	elix_file_write_formatted(&file, "default %s\n", defaults);
	elix_file_close(&file);

	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++){
		free(ini_list[i].buffer);
	}

	elix_file default_file;
	elix_file_open(&default_file, "build.ninja", EFF_FILE_TRUNCATE);
	elix_file_write_formatted(&default_file, "include %s\n\n", filename);
	elix_file_close(&default_file);

	return 0;
}

uint32_t fg_build_shellscript(CompilerInfo * target, CurrentConfiguration * options, char * filename ){
	LOG_INFO("Building: %s", filename);
	
	JoinConfigList ini_list[] = {
		{ "-D%s ", &options->defines, "-D", nullptr},
		{ "%s ", &options->flags, nullptr, nullptr},
		{ "-l%s ", &options->libs, "-l", nullptr},
		{ "%s ", &options->lib_flags, nullptr, nullptr},
		{ "%s ", &options->final_flags, nullptr, nullptr},
		{ "-I%s ", &options->includes, "-I", nullptr},
	};
	char platform_file[128] = "./config/$platform-common.txt";
	char arch_file[128] = "./config/$platform-$arch.txt";
	char compiler_file[128] = "./config/$platform-$arch-$compiler.txt";
	char compiler_common_file[128] = "./config/$compiler-$buildmode.txt";


	update_string_from_compilerinfo(platform_file, 128, target);
	update_string_from_compilerinfo(arch_file, 128, target);
	update_string_from_compilerinfo(compiler_file, 128, target);
	update_string_from_compilerinfo(compiler_common_file, 128, target);



	//Read Default
	parse_txtcfg("./config/default.txt", options, target);
	parse_txtcfg("./config/project.txt", options, target);
	parse_txtcfg(platform_file, options, target);
	parse_txtcfg(arch_file, options, target);
	parse_txtcfg(compiler_common_file, options, target);
	parse_txtcfg(compiler_file, options, target);
/*
	if ( find_configmap(&options->options, "DEBUG") != SIZE_MAX || find_configmap(&options->options, "debug") != SIZE_MAX) {
		push_configlist(&options->flags, "-g3");
		push_configlist(&options->lib_flags, "-g3");
		push_configlist(&options->defines, "_DEBUG");
	}
*/

	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++) {
		join_config_option(&ini_list[i]);
	}

	///NOTE: Workaround for -static-libstdc++ arg in libs
	if ( is_in_configlist(&options->lib_flags, "-static-libstdc++"))
	{
		char * buffer = get_configmap(&options->options, "linker");
		LOG_INFO("\tUsing -static-libstdc++ requires using g++ instead of gcc as linker.");
		elix_cstring_inreplace(buffer, 128, "gcc", "g++",0);
	}


	//Write
	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_TRUNCATE);

	elix_file_write_string(&file, "#!/bin/bash\n", 0);	

	elix_file_write_string(&file, "set -e\n", 0);

	elix_file_write_string(&file, "#Variables\n", 0);

	elix_file_write_formatted(&file, "compiler_lib=\"%s\"\n", ini_list[JCO_LIBS].buffer);
	elix_file_write_formatted(&file, "compiler_lib_flags=\"%s\"\n", ini_list[JCO_LIBS_FLAGS].buffer);
	elix_file_write_formatted(&file, "compiler_flags=\"%s\"\n", ini_list[JCO_FLAGS].buffer);
	elix_file_write_formatted(&file, "compiler_includes=\"%s\"\n", ini_list[JCO_INCLUDE].buffer);
	elix_file_write_formatted(&file, "compller_defines=\"%s\"\n", ini_list[JCO_DEFINES].buffer);
	elix_file_write_string_from_compilerinfo(&file, "compiler_mode=\"$mode\"\n", target);

	elix_file_write_formatted(&file, "finaliser_flags=\"%s\"\n", ini_list[JCO_FINAL_FLAGS].buffer);

	elix_file_write_string(&file, "binary_prefix=\"bin/\"\n", 0);
	elix_file_write_string_from_compilerinfo(&file, "binary_suffix=\"-$mode-$platform-$arch\"\n", target);

	char * required_options[] = {
		"finaliser", "compiler", "linker", "static_linker",
		"program_suffix", "finalise_suffix",
		"shared_suffix", "static_suffix"
	};

	for (size_t var = 0; var < ARRAY_SIZE(required_options); var++) {
		char * buffer = get_configmap(&options->options, required_options[var]);
		if ( buffer ) {
			elix_file_write_formatted(&file, "%s=\"%s\"\n", required_options[var], buffer);
		}
	}
	
	elix_file_write_string_from_compilerinfo(&file, "object_dir=\"compile/$platform-$arch/$mode/\"\n", target);
	elix_file_write_string(&file, "\n", 1);
	elix_file_write_string(&file, "function clean_command {\n", 0);
	elix_file_write_string(&file, "\techo \"TODO - Clean command is not complete\"\n", 0);
	elix_file_write_string(&file, "}\n", 2);

	elix_file_write_string(&file, "function run_command {\n", 0);
	elix_file_write_string(&file, "\techo \"TODO - Run command is not complete\"\n", 0);
	elix_file_write_string(&file, "}\n", 2);

	elix_file_write_string(&file, "\n#Files\n", 0);

	char * program_name = get_configmap(&options->options, "name");

	char objects[6144] = {0};
	char resources[6144] = {0};
	char defaults[1024] = {0};

	///TODO: finished resource handling in batch file
	/*
	if ( options->resources.current ) {
		for (size_t i = 0; i < options->resources.current; i++){
			for (size_t var = 5; var < ARRAY_SIZE(ninja_extension_output); var ++) {
					if ( elix_cstring_has_suffix(options->resources.value[i], ninja_extension_output[var].stage) ) {
						size_t leng = elix_cstring_length(options->resources.value[i], 0);
						size_t basename_leng = leng;
						char * basename = nullptr;
						for (size_t split = 0; split < leng; split++) {
							if ( options->resources.value[i][split] == '.') {
								basename_leng = split +1;
								break;
							}
						}
						basename = elix_cstring_from(options->resources.value[i], "NUL", basename_leng);
						elix_file_write_formatted(&file, ninja_extension_output[var].content, basename, options->resources.value[i]);
						elix_file_write_formatted(&file, "\n");

						char format_buffer[128] = {0};
						size_t q = snprintf(format_buffer, 128, "%s%s", basename, ninja_extension_output[var].oext);
						elix_cstring_append(resources, 6144, format_buffer, q);
						free(basename);
						break;
					}
			}

		}
		elix_file_write_formatted(&file, ninja_extension_output[3].content, resources);
		elix_cstring_append(defaults, 255, "${object_dir}/resources ", 24);
	}
	*/

	for (size_t j = 0; j < options->modules.current; j++){
		size_t i = options->files.current;
		ConfigList link_objects = {0};
		uint8_t linking_method = parse_filelist(options->modules.value[j], &options->files, &options->modules, &link_objects);

		char objects_buffer[6144] = {0};
		char * current_name = j == 0 ? program_name : options->modules.value[j];
		char last_directory[255] = {0};

		for (; i < options->files.current; i++){
			for (size_t var = 5; var < ARRAY_SIZE(bash_extension_output); var ++) {
				if ( !bash_extension_output[var].stage[0] ) {
					break;
				}
				if ( elix_cstring_has_suffix(options->files.value[i], bash_extension_output[var].stage) ) {
					size_t leng = elix_cstring_length(options->files.value[i], 0);
					for (size_t split = leng; split > 0; split--) {
						if ( options->files.value[i][split] == '.') {
							options->files.value[i][split] = 0;
							leng = split;
							break;
						}
					}
					
					char * no_src_dir = options->files.value[i];
					size_t ifrst_dir_index = elix_cstring_find_of(options->files.value[i], "/", 0);
					if ( ifrst_dir_index < 64 ) {
						no_src_dir  = options->files.value[i] + ifrst_dir_index;
					}

					size_t q = 0;
					char format_buffer[1024] = {0};
					
					size_t p = elix_cstring_find_last_of(no_src_dir, "/", 1 );
					if ( p != SIZE_MAX ) {
						no_src_dir[p] = 0; // NOTE: Stupid piece of code
						if ( !elix_cstring_match( last_directory, no_src_dir, 255) ) {
							q = snprintf(format_buffer, 1024, "\tmkdir -p ${object_dir}%s\n", no_src_dir);
							elix_cstring_append(objects_buffer, 6144, format_buffer, q);
							elix_cstring_copy(no_src_dir, last_directory);
						}
						no_src_dir[p] = '/'; // NOTE: Stupid piece of code
					}

					q = snprintf(format_buffer, 1024, bash_extension_output[var].content, no_src_dir, bash_extension_output[var].oext, options->files.value[i]);
					elix_cstring_append(objects_buffer, 6144, format_buffer, q);
					
					q = snprintf(format_buffer, 1024, "${object_dir}%s.%s ", no_src_dir, bash_extension_output[var].oext);
					elix_cstring_append(objects, 6144, format_buffer, q);

					break;
				}
			}
		}

		switch (linking_method)
		{
			case LT_STATIC:
				LOG_INFO("\tStatic Lib: %s", current_name);
				elix_file_write_formatted(&file, "function compile_%s {\n", current_name);
				elix_file_write_formatted(&file, "\techo 'compiling %s'\n", current_name);
				elix_file_write_string(&file, "\tmkdir -p ${object_dir}\n", 24);

				elix_file_write_string(&file, objects_buffer, 0);
				elix_file_write_formatted(&file, bash_extension_output[2].content, current_name, objects);
				elix_file_write_string(&file, "}\n", 0);

				elix_cstring_append(defaults, 1024, "\tcompile_", 10);
				elix_cstring_append(defaults, 1024, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 1024, "\n", 1);
				break;
			case LT_SHARED:
				LOG_INFO("\tShared Lib: %s", current_name);
				elix_file_write_formatted(&file, "function compile_%s {\n", current_name);
				elix_file_write_formatted(&file, "\techo 'compiling %s'\n", current_name);
				elix_file_write_string(&file, "\tmkdir -p ${object_dir}\n", 24);
				elix_file_write_string(&file, objects_buffer, 0);
				elix_file_write_formatted(&file, bash_extension_output[1].content, current_name, objects);
				elix_file_write_string(&file, "}\n", 0);

				elix_cstring_append(defaults, 1024, "\tcompile_", 10);
				elix_cstring_append(defaults, 1024, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 1024, "\n", 1);

				break;
			case LT_SKIP:
				LOG_INFO("\tModule Skipped: %s", current_name);
				break;
			default:
				LOG_INFO("\tProgram:sd %s", current_name);

				elix_file_write_formatted(&file, "function compile_%s {\n", current_name);
				elix_file_write_formatted(&file, "\techo 'compiling %s'\n", current_name);
				elix_file_write_string(&file, "\tmkdir -p ${object_dir}\n", 24);
				elix_file_write_string(&file, objects_buffer, 0);
				elix_file_write_formatted(&file, bash_extension_output[0].content, current_name, objects);
				elix_file_write_string(&file, "}\n", 0);

				elix_cstring_append(defaults, 1024, "\tcompile_", 10);
				elix_cstring_append(defaults, 1024, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 1024, "\n", 1);

				break;
		}
		elix_file_write_string(&file, "\n", 1);
		objects[0] = 0;
	}

	elix_file_write_string(&file, "\n", 1);
	///TODO: Finished this as well
	/*
	if ( find_configmap(&options->options, "finaliser") != SIZE_MAX ) {
		elix_file_write_formatted(&file, bash_extension_output[2].content, program_name, program_name);
	} else {

	}
	*/

	for (size_t j = 0; j < options->modules.current; j++){
		char * current_name = j == 0 ? program_name : options->modules.value[j];
		if ( 0 == j ) {
			elix_file_write_formatted(&file,"if [[ \"$1\" == \"%s\" ]]; then\n", current_name);
			elix_file_write_formatted(&file,"\tcompile_%s\n", current_name);
		} else {
			elix_file_write_formatted(&file,"elif [[ \"$1\" == \"%s\" ]]; then\n", current_name);
			elix_file_write_formatted(&file,"\tcompile_%s\n", current_name);
		}
	}
	elix_file_write_formatted(&file,"else\n");
	elix_file_write_formatted(&file,"%s", defaults);
	elix_file_write_formatted(&file,"fi\n");

	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++){
		free(ini_list[i].buffer);
	}

	return 0;
}

uint32_t fg_build_batch(CompilerInfo * target, CurrentConfiguration * options, char * filename ){
	LOG_INFO("Building: %s", filename);
	
	JoinConfigList ini_list[] = {
		{ "-D%s ", &options->defines, "-D", nullptr},
		{ "%s ", &options->flags, nullptr, nullptr},
		{ "-l%s ", &options->libs, "-l", nullptr},
		{ "%s ", &options->lib_flags, nullptr, nullptr},
		{ "%s ", &options->final_flags, nullptr, nullptr},
		{ "-I%s ", &options->includes, "-I", nullptr},
	};
	char platform_file[128] = "./config/$platform-common.txt";
	char arch_file[128] = "./config/$platform-$arch.txt";
	char compiler_common_file[128] = "./config/$compiler-$buildmode.txt";
	char compiler_file[128] = "./config/$platform-$arch-$compiler.txt";


	update_string_from_compilerinfo(platform_file, 128, target);
	update_string_from_compilerinfo(arch_file, 128, target);
	update_string_from_compilerinfo(compiler_file, 128, target);
	update_string_from_compilerinfo(compiler_common_file, 128, target);

	//Read Default
	parse_txtcfg("./config/default.txt", options, target);
	parse_txtcfg("./config/project.txt", options, target);
	parse_txtcfg(platform_file, options, target);
	parse_txtcfg(arch_file, options, target);
	parse_txtcfg(compiler_common_file, options, target);
	parse_txtcfg(compiler_file, options, target);

	if ( find_configmap(&options->options, "DEBUG") != SIZE_MAX || find_configmap(&options->options, "debug") != SIZE_MAX) {
		push_configlist(&options->flags, compiler_debug_options[0].flag);
		push_configlist(&options->lib_flags, compiler_debug_options[0].libflag);
		push_configlist(&options->defines, compiler_debug_options[0].define);
	}

	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++) {
		join_config_option(&ini_list[i]);
	}

	///NOTE: Workaround for -static-libstdc++ arg in libs
	if ( is_in_configlist(&options->lib_flags, "-static-libstdc++") ) {
		char * buffer = get_configmap(&options->options, "linker");
		LOG_INFO("\tUsing -static-libstdc++ requires using g++ instead of gcc as linker.");
		elix_cstring_inreplace(buffer, 128, "gcc", "g++", 0);
	}

	//Write
	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_TRUNCATE);

	elix_file_write_string(&file, "@echo off\n", 0);

	elix_file_write_string(&file, "rem Variables\n", 0);

	elix_file_write_formatted(&file, "set compiler_lib=%s\n", ini_list[JCO_LIBS].buffer);
	elix_file_write_formatted(&file, "set compiler_lib_flags=%s\n", ini_list[JCO_LIBS_FLAGS].buffer);
	elix_file_write_formatted(&file, "set compiler_flags=%s\n", ini_list[JCO_FLAGS].buffer);
	elix_file_write_formatted(&file, "set compiler_includes=%s\n", ini_list[JCO_INCLUDE].buffer);
	elix_file_write_formatted(&file, "set compller_defines=%s\n", ini_list[JCO_DEFINES].buffer);
	elix_file_write_string_from_compilerinfo(&file, "set compiler_mode=$mode\n", target);

	elix_file_write_formatted(&file, "set finaliser_flags=%s\n", ini_list[JCO_FINAL_FLAGS].buffer);

	elix_file_write_string(&file, "set binary_prefix=bin/\n", 0);
	elix_file_write_string_from_compilerinfo(&file, "set binary_suffix=-$mode-$platform-$arch\n", target);

	char * required_options[] = {
		"finaliser", "compiler", "linker", "static_linker",
		"program_suffix", "finalise_suffix",
		"shared_suffix", "static_suffix"
	};

	for (size_t var = 0; var < ARRAY_SIZE(required_options); var++) {
		char * buffer = get_configmap(&options->options, required_options[var]);
		if ( buffer ) {
			elix_file_write_formatted(&file, "set %s=%s\n", required_options[var], buffer);
		}
	}
	
	elix_file_write_string_from_compilerinfo(&file, "set object_dir=\"compile/$platform-$arch/$mode/\"\n", target);
	elix_file_write_string(&file, "\n", 1);

	elix_file_write_string(&file, "goto Commands\n\n", 0);


	elix_file_write_string(&file, ":run\n", 0);
	elix_file_write_string(&file, "\techo \"TODO\"\n", 0);


	elix_file_write_string(&file, ":clean_command\n", 0);
	elix_file_write_string(&file, "\techo \"TODO\"\n", 0);


	char * program_name = get_configmap(&options->options, "name");

	char objects[6144] = {0};
	char resources[6144] = {0};
	char defaults[1024] = {0};

	///TODO: finished resource handling in batch file
	/*
	if ( options->resources.current ) {
		for (size_t i = 0; i < options->resources.current; i++){
			for (size_t var = 5; var < ARRAY_SIZE(ninja_extension_output); var ++) {
					if ( elix_cstring_has_suffix(options->resources.value[i], ninja_extension_output[var].stage) ) {
						size_t leng = elix_cstring_length(options->resources.value[i], 0);
						size_t basename_leng = leng;
						char * basename = nullptr;
						for (size_t split = 0; split < leng; split++) {
							if ( options->resources.value[i][split] == '.') {
								basename_leng = split +1;
								break;
							}
						}
						basename = elix_cstring_from(options->resources.value[i], "NUL", basename_leng);
						elix_file_write_formatted(&file, ninja_extension_output[var].content, basename, options->resources.value[i]);
						elix_file_write_formatted(&file, "\n");

						char format_buffer[128] = {0};
						size_t q = snprintf(format_buffer, 128, "%s%s", basename, ninja_extension_output[var].oext);
						elix_cstring_append(resources, 6144, format_buffer, q);
						free(basename);
						break;
					}
			}

		}
		elix_file_write_formatted(&file, ninja_extension_output[3].content, resources);
		elix_cstring_append(defaults, 255, "${object_dir}/resources ", 24);
	}
	*/

	for (size_t j = 0; j < options->modules.current; j++) {
		size_t i = options->files.current;
		ConfigList link_objects = {0};
		uint8_t linking_method = parse_filelist(options->modules.value[j], &options->files, &options->modules, &link_objects);

		char objects_buffer[4096] = {0};
		char * current_name = j == 0 ? program_name : options->modules.value[j];

		for (; i < options->files.current; i++){
			for (size_t var = 5; var < ARRAY_SIZE(batch_extension_output); var ++) {
				if ( !batch_extension_output[var].stage[0] ) {
					break;
				}
				if ( elix_cstring_has_suffix(options->files.value[i], batch_extension_output[var].stage) ) {
					size_t leng = elix_cstring_length(options->files.value[i], 0);
					for (size_t split = leng; split > 0; split--) {
						if ( options->files.value[i][split] == '.') {
							options->files.value[i][split] = 0;
							leng = split;
							break;
						}
					}
					
					char * no_src_dir = options->files.value[i];
					size_t ifrst_dir_index = elix_cstring_find_of(options->files.value[i], "/", 0);
					if ( ifrst_dir_index < 64 ) {
						no_src_dir  = options->files.value[i] + ifrst_dir_index;
					}

					size_t q = 0;
					char format_buffer[1024] = {0};
					
					
					q = snprintf(format_buffer, 1024, batch_extension_output[var].content, no_src_dir, batch_extension_output[var].oext, options->files.value[i]);
					elix_cstring_append(objects_buffer, 6144, format_buffer, q);
					
					q = snprintf(format_buffer, 1024, "${object_dir}%s.%s ", no_src_dir, batch_extension_output[var].oext);
					elix_cstring_append(objects, 6144, format_buffer, q);

					break;
				}
			}
		}

		switch (linking_method)
		{
			case LT_STATIC:
				LOG_INFO("\tStatic Lib: %s", current_name);
				elix_file_write_formatted(&file, ":compile_%s \n", current_name);
				elix_file_write_string(&file, objects_buffer, 0);
				elix_file_write_formatted(&file, batch_extension_output[2].content, current_name, objects, current_name,current_name);
				elix_file_write_string(&file, "\tGOTO Exit\n", 0);

				elix_cstring_append(defaults, 1024, "compile_", 10);
				elix_cstring_append(defaults, 1024, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 1024, " ", 1);
				break;
			case LT_SHARED:
				LOG_INFO("\tShared Lib: %s", current_name);
				elix_file_write_formatted(&file, ":compile_%s \n", current_name);
				elix_file_write_string(&file, objects_buffer, 0);
				elix_file_write_formatted(&file, batch_extension_output[1].content, current_name, objects);
				elix_file_write_string(&file, "\tGOTO Exit\n", 0);

				elix_cstring_append(defaults, 1024, "compile_", 10);
				elix_cstring_append(defaults, 1024, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 1024, " ", 1);

				break;
			case LT_SKIP:
				LOG_INFO("\tModule Skipped: %s", current_name);
				break;
			default:
				LOG_INFO("\tProgram: %s", current_name);

				elix_file_write_formatted(&file, ":compile_%s \n", current_name);
				elix_file_write_string(&file, objects_buffer, 0);
				elix_file_write_formatted(&file, batch_extension_output[0].content, current_name, objects);
				elix_file_write_string(&file, "\tGOTO Exit\n", 0);

				elix_cstring_append(defaults, 1024, "compile_", 10);
				elix_cstring_append(defaults, 1024, current_name, elix_cstring_length(current_name, 0));
				elix_cstring_append(defaults, 1024, " ", 1);

				break;
		}
		elix_file_write_string(&file, "\n", 1);
		objects[0] = 0;
	}

	elix_file_write_string(&file, "\n:Commands\n", 0);
	///TODO: Finished this as well
	/*
	if ( find_configmap(&options->options, "finaliser") != SIZE_MAX ) {
		elix_file_write_formatted(&file, batch_extension_output[2].content, program_name, program_name);
	} else {

	}
	*/

	for (size_t j = 0; j < options->modules.current; j++){
		char * current_name = j == 0 ? program_name : options->modules.value[j];
		elix_file_write_formatted(&file,"if \"%%1\" == \"%s\" call :compile_%s \n", current_name, current_name);

	}
	elix_file_write_string(&file,"echo Arg '%1' not valid\n", 0 );
	elix_file_write_string(&file,":Exit\nexit", 0 );

	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++){
		free(ini_list[i].buffer);
	}

	return 0;
}


uint32_t fg_config_arch(CompilerInfo * target, CurrentConfiguration *options, char * filename ){
	update_string_from_compilerinfo(config_arch_text, 512, target);

	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_TRUNCATE);

	elix_file_write_string(&file, config_arch_text, 0);
	elix_file_close(&file);

	return 0;
}

uint32_t fg_config_default(CompilerInfo * target, CurrentConfiguration *options, char * filename ){
	elix_file file;

	if ( elix_file_open(&file, filename, EFF_FILE_TRUNCATE) == 0) {
		LOG_INFO("\t%s", filename);
	} else {
		LOG_INFO("\t[Failed] %s", filename);
		return 0;
	}
	elix_file_write_string(&file, config_default_text, 0);

	size_t pos = 0;
	lookup_configmap(&options->options, elix_cstring_hash("name"), &pos);
	if ( pos < 64 ) {
		elix_file_write_string(&file, "[options]\n", 0);
		elix_file_write_string(&file, "name=", 0);
		elix_file_write_string(&file, options->options.value[pos], 0);
		elix_file_write_string(&file, "\n", 0);
	}


	elix_file_close(&file);
	return 1;
}

uint32_t fg_config_platform(CompilerInfo * target, CurrentConfiguration *options, char * filename ){
	size_t index = 0;
	uint32_t os = elix_hash(target->os, elix_cstring_length(target->os, 0));
	for (size_t i = 0; i < ARRAY_SIZE(config_platform_text); i++) {
		if ( os == config_platform_text[i].id )
			index = i;
	}
	
	update_string_from_compilerinfo(config_platform_text[index].text, 1024, target);

	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_TRUNCATE);

	elix_file_write_string(&file, config_platform_text[index].text, 0);
	elix_file_close(&file);


	return 0;
}

uint32_t fg_null(CompilerInfo * target, CurrentConfiguration *options, char * filename ){
	return 0;
}

linking_type scan_directory(const char * directory_path) {
	linking_type lt = LT_PROGRAM;
	elix_directory * directory = elix_os_directory_list_files(directory_path, nullptr);
	
	if ( directory ) {
		for (size_t b = 0; b < directory->count; ++b) {
			if ( !elix_os_directory_is(directory->files[b].uri) ) {
				if ( !memcmp(directory->files[b].filename, ".shared", 8) ) {
					lt = LT_SHARED;
				} else if ( !memcmp(directory->files[b].filename, ".static", 8) ) {
					lt = LT_STATIC;
				}
			}
		}
		elix_os_directory_list_destroy(&directory);
	}
	
	return lt;
}

uint32_t scan_subdirectory( char * directory_path, elix_file * file) {
	if ( file == nullptr ) {
		LOG_INFO("\t Failed reading* : %s", directory_path);
		return 0;
	}
	elix_directory * current_dir = elix_os_directory_list_files(directory_path, nullptr);
	if ( current_dir ) {
		for (size_t a = 0; a < current_dir->count; ++a) {
			if ( !elix_os_directory_is(current_dir->files[a].uri) ) {
				if ( !memcmp(current_dir->files[a].filename, ".shared", 8) ) {
					elix_file_write_string(file, "$shared", 0);
				} else if ( !memcmp(current_dir->files[a].filename, ".static", 8) ) {
					elix_file_write_string(file, "$static", 0);
				} else {
					elix_file_write_string(file, current_dir->files[a].uri, 0);
				}
				elix_file_write_string(file, "\n", 1);
			} else {
				if ( !scan_subdirectory(current_dir->files[a].uri, file) ) {
					LOG_INFO("\t Failed reading : %s", current_dir->files[a].uri);
				}
			}
		}
		elix_os_directory_list_destroy(&current_dir);
		return 1;
	}
	LOG_INFO("\t Failed reading : %s", directory_path);
	return 0;
}

void directory_to_module( const char * module_name, char * directory_path, linking_type lt) {
	elix_file temp_file = {0};
	char temp_filename[512] = "./config/modules/$module.txt";
	elix_cstring_inreplace(temp_filename, 512, "$module", module_name, 0);
						
	if ( elix_file_open(&temp_file, temp_filename, EFF_FILE_TRUNCATE) == 0) {
		LOG_INFO("%s", temp_filename);
		scan_subdirectory(directory_path, &temp_file);
		elix_file_close(&temp_file);
	} else {
		LOG_INFO("[Failed] %s", temp_filename);
	}
}

uint32_t fg_wildcardfilelist(CompilerInfo * target, CurrentConfiguration *options, char * filename ) {
	elix_directory * defaults_dir = elix_os_directory_list_files("./"SOURCE_DIRECTORY"/", nullptr);

		elix_file file;
		if ( elix_file_open(&file, filename, EFF_FILE_TRUNCATE) == 0) {
			LOG_INFO("\t%s", filename);
			elix_file_write_string(&file, "./"SOURCE_DIRECTORY"/*.*\n", 0);
			elix_file_close(&file);
		} else {
			LOG_INFO("\t[Failed] %s", filename);
		}
	return 0;
}

uint32_t fg_defaultfilelist(CompilerInfo * target, CurrentConfiguration *options, char * filename ) {
	elix_directory * defaults_dir = elix_os_directory_list_files("./"SOURCE_DIRECTORY"/", nullptr);

	if ( defaults_dir ) {
		elix_file file;
		if ( elix_file_open(&file, filename, EFF_FILE_TRUNCATE) == 0) {
			LOG_INFO("\t%s", filename);

			for (size_t a = 0; a < defaults_dir->count; ++a) {
				if ( !elix_os_directory_is(defaults_dir->files[a].uri) ) {
					elix_file_write_string(&file, defaults_dir->files[a].uri, 0);
					elix_file_write_string(&file, "\n", 1);
				} else {
					linking_type lt = scan_directory(defaults_dir->files[a].uri);
					
					switch (lt)
					{
					case LT_SHARED:
						directory_to_module(defaults_dir->files[a].filename, defaults_dir->files[a].uri, lt);
						//elix_file_write_string(&file, "#", 0);
						//elix_file_write_string(&file, defaults_dir->files[a].filename, 0);
						//elix_file_write_string(&file, "\n", 0);
						break;
					case LT_STATIC:
						directory_to_module(defaults_dir->files[a].filename, defaults_dir->files[a].uri, lt);
						//elix_file_write_string(&file, "#", 0);
						//elix_file_write_string(&file, defaults_dir->files[a].filename, 0);
						//elix_file_write_string(&file, "\n", 0);
						break;
					default:
						scan_subdirectory(defaults_dir->files[a].uri, &file);
						break;
					}
					
					
				}
				
			}
			
		} else {
			LOG_INFO("\t[Failed] %s", filename);
		}
		elix_file_close(&file);
		elix_os_directory_list_destroy(&defaults_dir);
	} else {
		LOG_INFO("\t[Failed] %s", filename);
	}
	return 0;
}

void creeate_newproject(CompilerInfo * target, CurrentConfiguration *options) {
	char directories_project[][64] = {
		"./bin/",
		"./config/",
		"./config/modules/",
		"./include/",
		"./resources/",
		"./"SOURCE_DIRECTORY"/",
	};

	char file_generator[][128] = {
		"./config/default.txt",
		"./config/modules/base.txt",
	};

	uint32_t (*file_generator_function[])(CompilerInfo * target, CurrentConfiguration *options, char * filename ) = {
		&fg_config_default,
		&fg_wildcardfilelist,
		&fg_null,
	};

	LOG_INFO("Creating Directories:");
	for (size_t i = 0; i < ARRAY_SIZE(directories_project); i++) {
		
		if ( elix_os_directory_make(directories_project[i], 755, 1) ) {
			LOG_INFO("\t%s", directories_project[i]);
		} else {
			LOG_INFO("\t[Failed] %s", directories_project[i]);
		}
	}

	LOG_INFO("Creating Files:");

	elix_file file;
	if ( elix_file_open(&file, "./"SOURCE_DIRECTORY"/main.cpp", EFF_FILE_NEW) == 0) {
		LOG_INFO("\t./"SOURCE_DIRECTORY"/main.cpp");
		elix_file_write_string(&file, "#include <iostream>\nint main(int argc, char *argv[]) {\n\tstd::cout << \"Hello World\" << std::endl;\n}", 0);
	} 
	elix_file_close(&file);

	for (size_t i = 0; i < ARRAY_SIZE(file_generator); i++) 	{
		elix_cstring_inreplace(file_generator[i], 128, "$platform", target->os, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$arch", target->arch, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$compiler", target->compiler, 1);

		file_generator_function[i](target, options, file_generator[i]);
	}
}

void creeate_generator(CompilerInfo * target, CurrentConfiguration *options) {
	
	uint8_t batch_mode = target->build_system;
	elix_file file_check;
	elix_file_open(&file_check, "./config/default.txt", EFF_FILE_READ);
	if ( file_check.flag & EFF_FILE_READ_ERROR ) {
		LOG_INFO("Error: Run with the -platform argument");
		return;
	}
	elix_file_close(&file_check);

	char directories_generator[][128] = {
		"./compile/",
		"./compile/$platform-$arch",
		"./bin/"
	};

	char file_generator[][128] = {
		"./compile/$platform-$arch/build.ninja",
		"./run-build-$platform-$mode-$arch.bat",
		"./run-build-$platform-$mode-$arch.sh",
	};

	uint32_t (*file_generator_function[])(CompilerInfo * target, CurrentConfiguration *options, char * filename ) = {
		&fg_build_ninja,
		&fg_build_batch,
		&fg_build_shellscript,
		&fg_null,
		&fg_null,
	};


	if ( find_configmap(&options->options, "DEBUG") != SIZE_MAX || find_configmap(&options->options, "debug") != SIZE_MAX ) {
		push_configlist(&options->flags, compiler_debug_options[0].flag);
		push_configlist(&options->lib_flags, compiler_debug_options[0].libflag);
		push_configlist(&options->defines, compiler_debug_options[0].define);
	}


	LOG_INFO("Creating Directories");
	for (size_t i = 0; i < ARRAY_SIZE(directories_generator); i++) 	{
		elix_cstring_inreplace(directories_generator[i], 128, "$platform", target->os, 1);
		elix_cstring_inreplace(directories_generator[i], 128, "$arch", target->arch, 1);
		elix_cstring_inreplace(directories_generator[i], 128, "$compiler", target->compiler, 1);

		LOG_INFO("\t%s", directories_generator[i]);
		elix_os_directory_make(directories_generator[i], 755, 1);
	}

	for (size_t i = 0; i < ARRAY_SIZE(file_generator); i++) 	{
		elix_cstring_inreplace(file_generator[i], 128, "$platform", target->os, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$arch", target->arch, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$compiler", target->compiler, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$mode", target->mode, 1);
	
		if ( i == batch_mode ) {
			file_generator_function[i](target, options, file_generator[i]);
		}
	}

}

void creeate_newplatform(CompilerInfo * target, CurrentConfiguration *options) {
	char directories_generator[][128] = {
		"./compile/",
		"./compile/$platform-$arch",
		"./config/",
		"./config/modules/",
	};

	char file_generator[][128] = {
		"./config/$platform-$arch.txt",
		"./config/$platform-common.txt",
	};

	uint32_t (*file_generator_function[])(CompilerInfo * target, CurrentConfiguration *options, char * filename ) = {
		&fg_config_arch,
		&fg_config_platform,
		&fg_null,
		&fg_null,
		&fg_null,
		&fg_null,
		&fg_null,
	};

	LOG_INFO("Creating Directories:");
	for (size_t i = 0; i < ARRAY_SIZE(directories_generator); i++) 	{
		elix_cstring_inreplace(directories_generator[i], 128, "$platform", target->os, 1);
		elix_cstring_inreplace(directories_generator[i], 128, "$arch", target->arch, 1);
		elix_cstring_inreplace(directories_generator[i], 128, "$compiler", target->compiler, 1);

		LOG_INFO("\t%s", directories_generator[i]);
		elix_os_directory_make(directories_generator[i], 755, 1);
	}

	LOG_INFO("Creating Files");
	for (size_t i = 0; i < ARRAY_SIZE(file_generator); i++) 	{
		elix_cstring_inreplace(file_generator[i], 128, "$platform", target->os, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$arch", target->arch, 1);
		elix_cstring_inreplace(file_generator[i], 128, "$compiler", target->compiler, 1);

		LOG_INFO("\t%s", file_generator[i]);
		file_generator_function[i](target, options, file_generator[i]);
	}
}

void creeate_editorfiles(CompilerInfo * target, CurrentConfiguration *options, uint32_t editor_id) {
	char name[5] = UNPACK_ID(editor_id);

	LOG_INFO("Creating Editor files for %s", name);

	elix_file file;
	for (size_t i = 0; i < ARRAY_SIZE(editor_files); i++) 	{
		if (editor_files[i].id == editor_id) {
			LOG_INFO("\t%s", editor_files[i].filename);
			if ( elix_cstring_has_suffix(editor_files[i].filename, "/") ) {
				elix_os_directory_make(editor_files[i].filename, 755, 1);
			} else {
				if ( elix_file_open(&file, editor_files[i].filename, EFF_FILE_NEW) == 0) {
					elix_file_write_string(&file, editor_files[i].content, 0);
				} else {
					LOG_INFO("\t[Failed] %s", editor_files[i].filename);
				}
				elix_file_close(&file);
			}
		}
	}

}


void bin_2_header( const char * source_file, const char * target_file, const char * variable_name ) {
	uint8_t c;
	size_t file_size = 0;
	elix_file source_stream, target_stream;

	elix_file_open( &target_stream, target_file, EFF_FILE_TRUNCATE );
	elix_file_open( &source_stream, source_file, EFF_FILE_READ );

	elix_file_write_formatted(&target_stream, "size_t %s_size = %d; \n", variable_name, source_stream.length);
	elix_file_write_formatted(&target_stream, "uint8_t %s_data[%d] = { \n\t", variable_name, source_stream.length);
	
	while ( !elix_file_at_end(&source_stream) ) {
		elix_file_read(&source_stream, &c, 1, 1);
		elix_file_write_formatted(&target_stream, "0x%02x%c", c, (!elix_file_at_end(&source_stream) ? ',' : ' ') );
		if ( !(++file_size % 20) )
			elix_file_write_formatted(&target_stream, "\n\t");

	}
	elix_file_write_formatted(&target_stream, "\n};");

	elix_file_close(&target_stream);
	elix_file_close(&source_stream);
}

void print_project(CompilerInfo * target) {
	LOG_INFO("---- [Project] ----");
	LOG_INFO("---- [Platform] ----");
	elix_file build_file;
	if ( elix_file_open(&build_file, "./build.ninja", EFF_FILE_READ) ) {


		elix_file_close(&build_file);
	}

	elix_directory * directory = elix_os_directory_list_files("./config/", ".txt");
	if ( directory ) {
		for (size_t b = 0; b < directory->count; ++b) {
			if ( !elix_os_directory_is(directory->files[b].uri) ) {
				LOG_INFO("%s", directory->files[b].filename);
			}
		}
		elix_os_directory_list_destroy(&directory);
	}

}

void print_help(CompilerInfo * target) {
	CompilerInfo system = check_preprocessor();
	LOG_INFO("Configure Simple Builds system");
	LOG_INFO("Reads files from ./config/ to produce configurations for ninja builds");
	LOG_INFO("Target: %s-%s on %s compiler [%s mode]", target->os, target->arch, target->compiler, target->mode);
	LOG_INFO("System: %s-%s on %s compiler [%s mode]", system.os, system.arch, system.compiler, system.mode);
	LOG_INFO("Command Argument:");
	LOG_INFO(" -batch\t\t\t - Create Batch/Shell script instead of Ninja Build (Not fully suypported)");
	LOG_INFO(" -new\t\t\t - Create New Project");
	LOG_INFO(" -platform\t\t - Add New Platform");
	LOG_INFO(" -gen\t\t\t - Generate Build scripts");
	LOG_INFO(" -editor\t\t\t - [Unfinished] Generate Editor files (Currently only for VSCode)");
	LOG_INFO(" -c=\t\t\t - Binary to Header, via external command");
	LOG_INFO(" example: ./genscript.exe -h=a.h -i=a.png.temp -c=\"picasso -o a.png.temp a.png\"");

	LOG_INFO("Set Target options with arguments:");
	LOG_INFO("\tPLATFORM={os}");
	LOG_INFO("\tPLATFORM_ARCH={arch}");
	LOG_INFO("\tPLATFORM_COMPILER={compiler}");
	LOG_INFO("\tTARGET_TRIPLET={compiler_prefix} aka 'x86_64-w64-mingw32-'");
	LOG_INFO("\tRELEASE or DEBUG\n");
	exit(0);
}


void print_auto_message(CompilerInfo * target) {
  CompilerInfo system = check_preprocessor();
  LOG_INFO("Configure Simple Builds system");
  LOG_INFO("It automatically goes thought 'New Project', 'New Platform' then 'Generate Build Files' Steps.");
  
}
// > CLI

typedef struct {
	char title[128];
	uint8_t key;
	uint8_t return_code;
	void (*menu_option)(CompilerInfo * target, CurrentConfiguration *options );
} CLIMenu;



static CLIMenu menu_root[4] = {
	{"Create New Project", '1', 255, &creeate_newproject},
	{"Generate config files for $platform-$arch", '2', 255, &creeate_newplatform},
	{"Generate Build Script for $platform-$arch", '3', 255, &creeate_generator},
	{"Exit", 'q', 0, nullptr},
};


uint32_t cli_check_input(CompilerInfo * target, CurrentConfiguration *options, char line[512], CLIMenu * menu, uint32_t menu_count) {
	uint32_t resul = 1;
	for (size_t i = 0; i < menu_count; i++) {
		if ( line[0] == menu[i].key ) {
			if ( menu[i].return_code == 255 ) {
				PRINT(" < Running : %s\n", menu[i].title);
				if ( menu[i].menu_option ) {
					menu[i].menu_option(target, options);
				}
				
			}
			resul = menu[i].return_code;
			break;
		}
			
	}

	return resul;
}

void cli_display(CompilerInfo * target, CurrentConfiguration *options, CLIMenu * menu, uint32_t menu_count) {
	PRINT("============================================\n");
	for (size_t i = 0; i < menu_count; i++) {
		elix_cstring_inreplace(menu[i].title, 128, "$platform", target->os, 1);
		elix_cstring_inreplace(menu[i].title, 128, "$arch", target->arch, 1);
		elix_cstring_inreplace(menu[i].title, 128, "$compiler", target->compiler, 1);
		PRINT(">%c. %s\n", menu[i].key, menu[i].title);
	}
	PRINT("============================================\n");
	PRINT("Select Options:");
}

void cli_loop(CompilerInfo * target, CurrentConfiguration *options) {
	char line[512];
	uint32_t status;

	do {
		cli_display(target, options, menu_root, ARRAY_SIZE(menu_root));
		
		scanf("%511s", &line);
		status = cli_check_input(target, options, line, menu_root, ARRAY_SIZE(menu_root));
	} while (status);
}

int main(int argc, char * argv[]) {
	CurrentConfiguration configuration = {0};

	char directory_buffer[512] = {0};

	#ifdef __WIN32__
	_getcwd(directory_buffer,511);
	#else
	getcwd(directory_buffer,511);
	#endif
	if ( directory_buffer[0] ) {
		size_t directory_spliter = 511;
		while ( directory_spliter <= 511 ) {
			if ( directory_buffer[directory_spliter] == '\\' || directory_buffer[directory_spliter] == '/') {
				//Note: Lazy code
				memset(directory_buffer, ' ', directory_spliter+1);
				elix_cstring_trim(directory_buffer);
				break;
			}

			if ( !directory_spliter ) {
				break;
			}
			directory_spliter--;
		}
	} else {
		elix_cstring_copy("UntitledProject", directory_buffer);
	}
	pushset_configmap(&configuration.options, "name", directory_buffer, 1);

	push_configlist(&configuration.modules, "base");

	program_mode current_program_mode = PM_AUTO;
	uint8_t batch = 0;
	uint8_t release_mode = 0;

	CompilerInfo info = check_preprocessor();

	for (uint8_t var = 1; var < argc; ++var) {
		if ( elix_cstring_has_prefix(argv[var],"-help") ) {
			current_program_mode = PM_HELP;
		} else if ( elix_cstring_has_prefix(argv[var],"--help") ) {
			current_program_mode = PM_HELP;
		} else if ( elix_cstring_has_prefix(argv[var],"-info") ) {
			current_program_mode = PM_INFO;
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM=")) {
			elix_cstring_copy(argv[var]+9, info.os);
			find_plaform_details(&info);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_ARCH=")) {
			elix_cstring_copy(argv[var]+14, info.arch);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_COMPILER=")) {
			elix_cstring_copy(argv[var]+18, info.compiler);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_LINKER=")) {
			//elix_cstring_copy(argv[var]+16, info.linker);
		} else if ( elix_cstring_has_prefix(argv[var],"TARGET_TRIPLET=")) {
			elix_cstring_copy(argv[var]+15, info.triplet);
		} else if ( elix_cstring_has_prefix(argv[var],"-batch") ) {
			///Note: info.os_hash should be base system, even if overwritten 
			if ( info.os_hash == 0xbf7dcc4 ) {
				batch = 1; //windows batch
			} else {
				batch = 2;// shell script
			}
		} else if ( elix_cstring_has_prefix(argv[var],"-new") ) {
			current_program_mode = PM_NEWPROJECT;
		} else if ( elix_cstring_has_prefix(argv[var],"-platform") ) {
			current_program_mode = PM_NEWPLATFORM;
		} else if ( elix_cstring_has_prefix(argv[var],"-modules") ) {
			current_program_mode = PM_UPDATEMODULE;
		} else if ( elix_cstring_has_prefix(argv[var],"-gen") ) {
			current_program_mode = PM_GEN;
		} else if ( elix_cstring_has_prefix(argv[var],"-self") ) {
			current_program_mode = PM_REBUILDSELF;
		} else if ( elix_cstring_has_prefix(argv[var],"-editor") ) {
			current_program_mode = PM_EDITORCONFIG;
		} else if ( elix_cstring_has_prefix(argv[var],"-switch") ) {
			current_program_mode = PM_SWITCH;
		} else if ( elix_cstring_has_prefix(argv[var],"-ask") ) {
			current_program_mode = PM_INTERACTIVE;
		} else if ( elix_cstring_has_prefix(argv[var],"-c=") ) {
			current_program_mode = PM_B2H;
			push_configmap(&configuration.options, argv[var], 0);
		} else {
			push_configmap(&configuration.options, argv[var], 0);
		}
	}

	if ( find_configmap(&configuration.options, "DEBUG") != SIZE_MAX || find_configmap(&configuration.options, "debug") != SIZE_MAX) {
		elix_cstring_copy("debug", info.mode);
	} else if ( find_configmap(&configuration.options, "RELEASE") != SIZE_MAX || find_configmap(&configuration.options, "release") != SIZE_MAX) {
		elix_cstring_copy("release", info.mode);
	} else {
		push_configmap(&configuration.options, info.mode, 0);
	}

	if ( current_program_mode == PM_AUTO ) {
		char check_platformarch[128] = "./config/$platform-$arch.txt";

		elix_cstring_inreplace(check_platformarch, 128, "$platform", info.os, 1);
		elix_cstring_inreplace(check_platformarch, 128, "$arch", info.arch, 1);

		elix_file file_check;
		if ( elix_file_open(&file_check, "./config/default.txt", EFF_FILE_READ) != 0) {
			current_program_mode = PM_NEWPROJECT;
		}
		elix_file_close(&file_check);

		if ( current_program_mode == PM_AUTO ) {
			if ( elix_file_open(&file_check, check_platformarch, EFF_FILE_READ) != 0 ) {
				current_program_mode = PM_NEWPLATFORM;
			}
			elix_file_close(&file_check);
		}
		if ( current_program_mode == PM_AUTO )
			current_program_mode = PM_GEN;
		print_auto_message(&info);
	}

	switch (current_program_mode) {
		case PM_EDITORCONFIG:
			LOG_INFO("---- [Generating Editor Files] ----");
			creeate_editorfiles(&info, &configuration, PACK_ID('V','S','C','o'));
			break;
		case PM_UPDATEMODULE:
			LOG_INFO("---- [Update Modules] ----");
			fg_defaultfilelist(&info, &configuration, "./config/modules/base.txt");
			LOG_INFO("Run '%s -gen' to update build scripts for %s-%s", argv[0], info.os, info.arch);
			break;
		case PM_NEWPROJECT:
			LOG_INFO("---- [New Project] ----");
			creeate_newproject(&info, &configuration);
			LOG_INFO("Run '%s -platform' to build config files for %s-%s", argv[0], info.os, info.arch);
			break;
		case PM_NEWPLATFORM:
			LOG_INFO("---- [Generating Config Files] ----");
			LOG_INFO("- Target: %s-%s on %s compiler [%s mode]  [ID:%x]", info.os, info.arch, info.compiler, info.mode,  elix_hash(info.os, elix_cstring_length(info.os, 0)));
			creeate_newplatform(&info, &configuration);
			break;
		case PM_GEN:
			LOG_INFO("---- [Generating Build Files] ----");
			LOG_INFO("- Target: %s-%s on %s compiler [%s mode]  [ID:%x]", info.os, info.arch, info.compiler, info.mode,  elix_hash(info.os, elix_cstring_length(info.os, 0)));
			info.build_system = batch;
			creeate_generator(&info, &configuration);
			LOG_INFO("Run ninja to build project");
			break;
		case PM_B2H:
			LOG_INFO("Binary to Header");
			char * command_string = get_configmap(&configuration.options, "-c");
			char * target_string = get_configmap(&configuration.options, "-h");
			char * source_string = get_configmap(&configuration.options, "-i");
			
			if ( command_string && command_string[0] == '"') {
				//NOTE: Debugging via VSCODE can't handle quoted args so I had to double up on quotes
				elix_cstring_dequote(command_string);
			}

			if ( command_string && target_string && source_string) {
				char ** arguments = elix_cstring_split( command_string, ' ', '"');
				if ( arguments ) {
					elix_path path = elix_path_create(target_string);

					//
					int err = 0;
					LOG_INFO("Running command");
					#ifdef __WIN32__
					err = _spawnvp( _P_WAIT, arguments[0], (const char *const *) arguments );
					#else
					pid_t pid;
					err = posix_spawnp(&pid, arguments[0], nullptr, nullptr, arguments, nullptr );
					#endif

					if ( err != 0 ) {
						LOG_INFO("Can not convert binary to header");
					} else {
						LOG_INFO("Converted binary to header");
						LOG_INFO("Source: %s - Target: %s - Variable: %s", source_string, target_string, path.filename);
						bin_2_header( source_string, target_string, path.filename );
					}
				}
			} else {
				LOG_INFO("Can not convert binary to header");
			}
			break;
		case PM_SWITCH:
			print_project(&info); 
			break;
		case PM_INFO:
			char * platform_list[] = {
				"windows", "linux", "bsd", "haiku", "unknown", "3ds", nullptr
			};

			LOG_INFO("---- [Platform Hash Table] ----");
			for (size_t i = 0; platform_list[i]; i++) {
				LOG_INFO("- %s\t\t0x%x", platform_list[i], elix_hash(platform_list[i], elix_cstring_length(platform_list[i], 0)) );
			}

			break;
		case PM_INTERACTIVE:
			cli_loop(&info, &configuration);
			break;
		default:
			print_help(&info);
			break;
	}

	return 0;
}
