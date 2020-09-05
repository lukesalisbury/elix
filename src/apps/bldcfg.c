#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef nullptr
	#define nullptr NULL
#endif

#define LOG_MESSAGE(M, ...) printf("%18s:%04d | " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define FH(q) q

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

typedef void* data_pointer;
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
		if ( file->pos == file->length )
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

static inline file_size elix_file_tell(elix_file * file) {
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
	file->handle = fopen(filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	//fopen_s( &file->handle, filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	if ( file->handle ) {
		file->flag = mode;
		elix_file__update_length(file);
		return true;
	} else {
		LOG_MESSAGE("File not found %s", filename);
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

#include <sys/types.h>
#include <dirent.h>

static inline size_t elix_cstring_length(const char * string, uint8_t include_terminator ) {
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

size_t elix_cstring_append( char * str, const size_t len, const char * text, const size_t text_len) {
	size_t length = elix_cstring_length(str, 0);
	// TODO: Switch to memcpy
	for (size_t c = 0;length < len && c < text_len; length++, c++) {
		str[length] = text[c];
	}
	str[length+1] = 0;
	return length;
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
	memcpy(dest, ptr, length);
	dest[length] = 0;
	return dest;
}


void elix_cstring_copy( const char * source_init, char * dest_init)
{
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
	} while(*source != 0);
	*dest = '\0';
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

#define ELIX_CHAR_LOWER 1
#define ELIX_CHAR_UPPER 2
#define ELIX_CHAR_CAPITALISE 3
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

typedef struct {
	char os[16];
	char arch[8];
	char compiler[8];
	uint8_t bits;
} CompilerInfo;

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
			//#define PLATFORM_BITS 64
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
			#define PLATFORM_COMPILER "GCC"
		#elif defined(__llvm__)
			#define PLATFORM_COMPILER "LLVM"
		#elif defined(_MSC_VER)
			#define PLATFORM_COMPILER "MSVC"
		#else
			#define PLATFORM_COMPILER "unknown"
		#endif
	#endif

	elix_cstring_copy(PLATFORM, info.os);
	elix_cstring_copy(PLATFORM_ARCH, info.arch);
	elix_cstring_copy(PLATFORM_COMPILER, info.compiler);
	info.bits = PLATFORM_BITS;

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

typedef enum {
	SM_NONE, SM_FILE, SM_DEFINES, SM_FLAGS, SM_LIBS, SM_SUPPORTS, SM_OTHER
} scan_mode;

void print_help(CompilerInfo * target) {
	CompilerInfo system = check_preprocessor();
	printf("Configure Simple Ninja builds \n");
	printf("Reads INI from ./buildscripts/settings/ to produce configurations for ninja builds\n");
	printf("Target: %s %s on %s compiler\n", target->os, target->arch, target->compiler);
	printf("System: %s %s on %s compiler\n", system.os, system.arch, system.compiler);
	printf("Set Target options with arguments: PLATFORM={os} PLATFORM_ARCH={arch} PLATFORM_COMPILER={compiler}\n\n");
	exit(0);
}

#define ARRAY_SIZE(Array) (sizeof(Array) / sizeof((Array)[0]))
int main(int argc, char * argv[]) {
	uint32_t options[66] = {0};

	uint8_t help = 0;
	uint8_t batch = 0;

	char defines[1024] = {0};
	char output[1024] = {0};
	char * extension_output[] = {
		"cpp", "build_cpp ${object_dir}/%s.o src/%s.cpp",
		"c", "build_c ${object_dir}/%s.o src/%s.c",
	};

	CompilerInfo info = check_preprocessor();

	options[++options[0]] = elix_hash("base", 4);
	for (uint8_t var = 1; var < argc; ++var) {
		if ( elix_cstring_has_prefix(argv[var],"-help") ) {
			help = 1;
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM=")) {
			elix_cstring_copy(argv[var]+9, info.os);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_ARCH=")) {
			elix_cstring_copy(argv[var]+14, info.arch);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_COMPILER=")) {
			elix_cstring_copy(argv[var]+18, info.compiler);
		} else if ( elix_cstring_has_prefix(argv[var],"-batch") ) {
			batch = 1;
		} else {
			if ( options[0] < 66 ) {
				options[++options[0]] = elix_hash(argv[var], elix_cstring_length(argv[var],0));
			}
		}
	}
	if ( help ) {
		print_help(&info);
	}

	printf("Target OS: %s %s\n", info.os, info.arch);
	{
		size_t position = 0;
		char buffer[64]= {};
		elix_cstring_append(buffer, 64, info.os, elix_cstring_length( info.os,0));
		position = elix_cstring_append(buffer, 64, "-common", 7);
		options[++options[0]] = elix_hash(buffer, elix_cstring_length(buffer,0));

		buffer[position-6] = 0;
		elix_cstring_append(buffer, 64, info.arch, elix_cstring_length( info.arch,1));
		options[++options[0]] = elix_hash(buffer, elix_cstring_length(buffer,0));

	}
	if ( options[0] ) {
		for (size_t a = 1; a < options[0]; ++a) {
			printf("Option: %d\n",options[a]);
		}
	}

	elix_directory * defaults_dir = elix_os_directory_list_files("./bldsetting/", ".default");
	elix_path platfom_file = {0};
	if ( defaults_dir ) {
		for (size_t a = 0; a < defaults_dir->count; ++a) {
			uint32_t hash = elix_hash(defaults_dir->files[a].filename, elix_cstring_length(defaults_dir->files[a].filename,0));
			printf("Platform: %s %d \n", defaults_dir->files[a].filename, hash);
		}
		elix_os_directory_list_destroy(&defaults_dir);
	}

	elix_directory * settings_dir = elix_os_directory_list_files("./bldsetting/", ".ini");
	if ( settings_dir ) {
		for (size_t a = 0; a < settings_dir->count; ++a) {
			uint32_t file_hash = 0;
			uint8_t file_include = 0;

			elix_cstring_transform(settings_dir->files[a].filename, ELIX_CHAR_LOWER);
			file_hash = elix_hash(settings_dir->files[a].filename, elix_cstring_length(settings_dir->files[a].filename,0));
			for (size_t ind = 1; ind < 66; ++ind) {
				if ( options[ind] == file_hash ) {
					printf("Include: %s\n", settings_dir->files[a].filename);
					file_include = 1;
				}
			}
			if ( file_include ) {
				elix_file file;
				elix_file_open(&file, settings_dir->files[a].uri, EFF_FILE_READ_ONLY);

				scan_mode mode = SM_NONE;
				while (!elix_file_at_end(&file)) {
					char data[256];
					elix_file_read_line(&file, data, 256);
					size_t data_length = elix_cstring_trim(data);
					if ( data[0] == '[') {
						if ( elix_cstring_has_prefix(data,"[defines]") ) {
							mode = SM_DEFINES;
						} else if ( elix_cstring_has_prefix(data,"[files]") ) {
							mode = SM_FILE;
						}
					} else if ( data[0] == '#') {
					} else if ( data[0] < 32 ) {

					} else {
						if ( mode == SM_FILE ) {
							for (size_t var = 0; var < ARRAY_SIZE(extension_output); var += 2) {
								if ( elix_cstring_has_suffix(data,extension_output[var]) ) {
									for (size_t split = data_length; split > 0; split--) {
										if ( data[split] == '.') {
											data[split] = 0;
											break;
										}
									}
									printf(extension_output[var+1],data, data);
									printf("\n");
									break;
								}
							}
						} else {
							printf("%d: %s\n",mode, data);
						}

					}

				}
				elix_file_close(&file);
			} else {
				printf("Skipped: '%s'\n", settings_dir->files[a].filename);
			}
		}
		elix_os_directory_list_destroy(&settings_dir);
	}

	return 0;
}
