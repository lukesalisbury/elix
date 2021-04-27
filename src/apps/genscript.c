/*************************************************************************
 * Generates build.ninja files for multiple platforms
 * build: gcc genscript.c -o genscript.exe
 * run: genscript.exe [-help]
 * 
 * 
*************************************************************************/
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

#include <unistd.h>
#if __WIN32__
#include <process.h>
#else
#include <spawn.h>
#endif

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
	EFF_FILE_WRITE = 1 << 4, EFF_FILE_WRITE_ERROR = 1 << 5, EFF_WRITABLE = 1 << 6,
	EFF_FILE_CREATE = 1 << 7
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

	char * dest = malloc(length); //new char[length]();
	elix_cstring_copy2(ptr, dest, length-1);
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
		uri.filename = elix_cstring_from(string + split, "/", extension_split - split + 1);
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


uint32_t elix_cstring_hash( const char * str ) {
	return elix_hash(str, elix_cstring_length(str, 0));
}

typedef struct {
	uint32_t id;
	char text[1024];
} PlatformConfig;

typedef struct {
	char stage[16];
	char oext[16];
	char content[256];
} ExtensionOutput;

static char config_arch_text[512] = "[defines]\n\
PLATFORM_BITS=$bits\n\
";

static char config_default_text[2048] = "[options]\n\
compiler=$tripletgcc\n\
linker=$tripletgcc\n\
name=testapp\n\
program_suffix=\n\
[options-windows]\n\
program_suffix=.exe\n\
[options-3ds]\n\
finaliser=3dsxtool\n\
program_suffix=.elf\n\
finalise_suffix=.3dsx\n\
[commands]\n\
compile_cpp=${compiler} ${compiler_includes} ${compller_defines} ${compiler_flags} -o $out -c $in\n\
compile_c=${compiler} ${compiler_includes} ${compller_defines}  ${compiler_flags} -o $out -c $in\n\
link_shared=${linker} -shared ${compiler_lib_flags} $in -o $out ${compiler_lib} \n\
link_static=${linker} -static ${compiler_lib_flags} $in -o $out ${compiler_lib}\n\
link=${linker} ${compiler_lib_flags} $in -o $out ${compiler_lib} \n\
finalise=${finaliser} ${finalise_flags} $in -o $out\n\
build_resources=echo\n\
clean=rm -rf ${object_dir}\n\
";

static ExtensionOutput ninja_extension_output[32] = {
	{"link_stage", "", "build ${binary_prefix}%s${binary_suffix}${program_suffix}: link %s\n"},
	{"resource_stage", "", "build ${object_dir}/resources: build_resources %s\n"},
	{"final_stage", "", "build ${binary_prefix}%s${binary_suffix}${finalise_suffix}: finalise ${binary_prefix}%s${binary_suffix}${program_suffix}\n"},
	{"cpp", "o", "build ${object_dir}%s.%s: compile_cpp %s.cpp\n"},
	{"c", "o", "build ${object_dir}%s.%s: compile_c %s.c\n"},
	{ "", "", ""}
};

static PlatformConfig config_platform_text[] = {
	{ 0, "[libs]\nstdc++\nm\n[lib_flags]\n-std=c++11\n\n[includes]\n[flags]\n-std=c++11\n[defines]\nPLATFORM_$PLATFORM\n[linker_flags]\n-Wl,-rpath -Wl,\\$$ORIGIN/lib\n"},
	{ 0x6986d1e1, "[libs]\nstdc++\nm\nctru\ncitro3d\n[lib_flags]\n-std=c++11\n-march=armv6k\n-mtune=mpcore\n-mfloat-abi=hard\n-mtp=soft\n-specs=3dsx.specs\n-L\"/opt/devkitpro/devkitARM/lib\"\n-L\"/opt/devkitpro/libctru/lib\"\n[includes]\n\"/opt/devkitpro/devkitARM/arm-none-eabi/include\"\n\"/opt/devkitpro/libctru/include\"\n[flags]\n-std=c++11\n-march=armv6k\n-mtune=mpcore\n-mfloat-abi=hard\n[defines]\nPLATFORM_3DS\nPLATFORM_BITS=32\nARM11\n[commands]\nfinalise=${finaliser} $in  $out ${finalise_flags}\nbuild_smdh=smdhtool --create -o $out\nbuild_pica=./genscript.exe -h=$out -i=$in.temp -c=\"picasso -o $in.temp $in\"\n[extension]\npica=h;build res/%s.h: build_pica %s.pica\n"}
};


typedef struct {
	char os[16];
	char arch[8];
	char compiler[8];
	char mode[8];
	char bits[4];
	char triplet[32];
} CompilerInfo;

typedef enum {
	SM_NONE, SM_FILE, SM_DEFINES, SM_FLAGS, SM_LIBS, SM_LIB_FLAGS, SM_FINAL_FLAGS, SM_INCLUDES, SM_MODULES, SM_OTHER
} scan_mode;

typedef enum {
	PM_HELP, PM_NEWPROJECT, PM_NEWPLATFORM, PM_GEN, PM_B2H, PM_AUTO
} program_mode;

typedef enum {
	JCO_DEFINES, JCO_FLAGS, JCO_LIBS, JCO_LIBS_FLAGS, JCO_FINAL_FLAGS, JCO_INCLUDE
} join_config_options;

typedef struct {
	uint8_t current;
	uint32_t hash[64];
	char key[64][256];
	char value[64][256];
} ConfigMap;


typedef struct {
	uint8_t current;
	char value[256][256];
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

void push_configlist(ConfigList * list, const char * data) {
	if (  list->current < 64 ) {
		size_t length = elix_cstring_length(data, 0);
		elix_cstring_append(list->value[list->current], 256, data, length);
		list->current++;
	}
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
	elix_cstring_copy("release", info.mode);

	snprintf(info.bits,3, "%d", PLATFORM_BITS);


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
	elix_cstring_inreplace(source_text, source_size, "$platform", target->os);
	elix_cstring_inreplace(source_text, source_size, "$arch", target->arch);
	elix_cstring_inreplace(source_text, source_size, "$compiler", target->compiler);
	elix_cstring_inreplace(source_text, source_size, "$mode", target->mode);
	elix_cstring_inreplace(source_text, source_size, "$bits", target->bits);
	elix_cstring_inreplace(source_text, source_size, "$triplet", target->triplet);

	char os[16];
	elix_cstring_copy(target->os, os);
	elix_cstring_transform(os, ELIX_CHAR_UPPER);
	elix_cstring_inreplace(source_text, source_size, "$PLATFORM", os);

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

void parse_filelist( const char * module, ConfigList * files) {
	char filename[512] = "./config/modules/$module.lst";

	elix_cstring_inreplace(filename, 512, "$module", module);

	elix_file file = {0};
	elix_file_open(&file, filename, EFF_FILE_READ_ONLY);
	while (!elix_file_at_end(&file)) {
		char data[256] = {};
		elix_file_read_line(&file, data, 256);
		elix_cstring_trim(data);
		if ( elix_cstring_has_prefix(data, "./") ) {
			elix_cstring_copy(data + 2, files->value[files->current]);
		} else {
			elix_cstring_copy(data, files->value[files->current]);
		}
		
		files->current++;
	}

}


void parse_txtcfg( const char * filename,  CurrentConfiguration * options, CompilerInfo * compiler) {
	LOG_INFO("Reading %s", filename);
	elix_file file = {0};
	ConfigList * write_options = nullptr;
	ConfigMap * write_map = nullptr;
	uint8_t write_mode = 0;
	char platform_option[64] = {0};
	char commands_option[64] = {0};
	snprintf(platform_option, 64, "[options-%s]", compiler->os);
	snprintf(commands_option, 64, "[commands-%s]", compiler->os);
	elix_file_open(&file, filename, EFF_FILE_READ_ONLY);
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
			} else {
				LOG_INFO("Config option skipped: %s", data);
			}
		}
	}
	elix_file_close(&file);

}





void print_help(CompilerInfo * target) {
	CompilerInfo system = check_preprocessor();
	LOG_INFO("Configure Simple Ninja builds ");
	LOG_INFO("Reads INI from ./buildscripts/settings/ to produce configurations for ninja builds");
	LOG_INFO("Target: %s %s on %s compiler", target->os, target->arch, target->compiler);
	LOG_INFO("System: %s %s on %s compiler", system.os, system.arch, system.compiler);
	LOG_INFO("Command Argument:");
	LOG_INFO("\t-batch\t - Create Batch/Shell script instead of Ninja Build");
	LOG_INFO("\t-new\t - Create New Project");
	LOG_INFO("\t-platform\t - Create New Platform");
	LOG_INFO("Set Target options with arguments:");
	LOG_INFO("\tPLATFORM={os}");
	LOG_INFO("\tPLATFORM_ARCH={arch}");
	LOG_INFO("\tPLATFORM_COMPILER={compiler}");
	LOG_INFO("\tPLATFORM_LINKER={linker}");
	LOG_INFO("\tRELEASE or DEBUG\n");
	exit(0);
}


size_t join_config_option( JoinConfigList * item) {
	if ( item->buffer == nullptr ) {
		item->buffer = malloc(1024);
	}

	memset(item->buffer, 0, 1024);
	
	for (size_t i = 0; i < item->option->current; i++) {
		char tempbuffer[128] = {0};
		size_t q = snprintf(tempbuffer, 128, item->formatting, item->option->value[i]);
		elix_cstring_append(item->buffer, 1024, tempbuffer, q);

	}
	
	return elix_cstring_length(item->buffer, 0);
}

uint32_t fg_build_ninja(CompilerInfo * target, CurrentConfiguration * options, char * filename ){
	LOG_INFO("Building %s", filename);
	
	JoinConfigList ini_list[] = {
		{ "-D%s ", &options->defines, nullptr},
		{ "%s ", &options->flags, nullptr},
		{ "-l%s ", &options->libs, nullptr},
		{ "%s ", &options->lib_flags, nullptr},
		{ "%s ", &options->final_flags, nullptr},
		{ "-I%s ", &options->includes, nullptr},
	};
	char platform_file[128] = "./config/$platform-common.txt";
	char arch_file[128] = "./config/$platform-$arch.txt";
	char compiler_file[128] = "./config/$platform-$arch-$compiler.txt";
	char compiler_common_file[128] = "./config/$compiler-common.txt";




	update_string_from_compilerinfo(platform_file, 128, target);
	update_string_from_compilerinfo(arch_file, 128, target);

	//Read Default
	parse_txtcfg("./config/default.txt", options, target);
	parse_txtcfg(platform_file, options, target);
	parse_txtcfg(arch_file, options, target);

	parse_filelist("base", &options->files);
	for (size_t i = 0; i < options->modules.current; i++){
		LOG_INFO("Module: %s",options->modules.value[i]);
		parse_filelist(options->modules.value[i], &options->files);
	}


	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++){
		join_config_option(&ini_list[i]);
	}

	//Write
	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_WRITE);


	elix_file_write_string(&file, "ninja_required_version = 1.3\n", 0);
	elix_file_write_string_from_compilerinfo(&file, "builddir=build/$platform-$arch/log\n", target);

	elix_file_write_formatted(&file, "compiler_lib = %s\n", ini_list[JCO_LIBS].buffer);
	elix_file_write_formatted(&file, "compiler_lib_flags = %s\n", ini_list[JCO_LIBS_FLAGS].buffer);
	elix_file_write_formatted(&file, "compiler_flags = %s\n", ini_list[JCO_FLAGS].buffer);
	elix_file_write_formatted(&file, "compiler_includes = %s\n", ini_list[JCO_INCLUDE].buffer);
	elix_file_write_formatted(&file, "compller_defines = %s\n", ini_list[JCO_DEFINES].buffer);
	elix_file_write_string_from_compilerinfo(&file, "compiler_mode = $mode\n", target);

	elix_file_write_formatted(&file, "finaliser_flags = %s\n", ini_list[JCO_FINAL_FLAGS].buffer);

	elix_file_write_string(&file, "binary_prefix = bin/\n", 0);
	elix_file_write_string_from_compilerinfo(&file, "binary_suffix = -$mode-$platform-$arch\n", target);


	char * required_options[] = {
		"finaliser", "compiler", "linker",
		"program_suffix", "finalise_suffix",
	};


	for (size_t var = 0; var < ARRAY_SIZE(required_options); var++) {
		char * buffer = get_configmap(&options->options, required_options[var]);
		if ( buffer ) {
			elix_file_write_formatted(&file, "%s = %s\n", required_options[var], buffer);
		}
	}


	
	elix_file_write_string_from_compilerinfo(&file, "object_dir = build/$platform-$arch/$mode\n", target);
	elix_file_write_string(&file, "\n", 1);
	//elix_file_write_string(&file,  default_ninja_rules, 0);

	for (size_t i = 0; i < options->commands.current; i++){
		elix_file_write_formatted(&file, "rule %s\n", options->commands.key[i]);
		elix_file_write_formatted(&file, "  command = %s\n", options->commands.value[i]);
		elix_file_write_formatted(&file, "  description = [%s] $in\n\n", options->commands.key[i]);
	}


	elix_file_write_string(&file, "\n", 1);
	elix_file_write_string(&file, "build clean: clean\n", 0);
	elix_file_write_string(&file, "\n", 1);
	elix_file_write_string(&file, "#Files\n", 0);

	char * program_name = get_configmap(&options->options, "name");

	char objects[6144] = {0};
	char resources[6144] = {0};
	char defaults[256] = {0};

	if ( options->resources.current ) {
		for (size_t i = 0; i < options->resources.current; i++){
			for (size_t var = 3; var < ARRAY_SIZE(ninja_extension_output); var ++) {
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
		elix_file_write_formatted(&file, ninja_extension_output[1].content, resources);
		elix_cstring_append(defaults, 255, "${object_dir}/resources ", 24);
	}


	for (size_t i = 0; i < options->files.current; i++){
		for (size_t var = 3; var < ARRAY_SIZE(ninja_extension_output); var ++) {
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
				size_t q = snprintf(format_buffer, 128, "${object_dir}%s.%s", no_src_dir, ninja_extension_output[var].oext);
				elix_cstring_append(objects, 6144, format_buffer, q);

				break;
			}
		}
	}




	


	elix_file_write_formatted(&file, ninja_extension_output[0].content, program_name, objects);

	if ( find_configmap(&options->options, "finaliser") != SIZE_MAX ) {
		elix_file_write_string(&file, "\n", 1);
		elix_file_write_formatted(&file, ninja_extension_output[2].content, program_name, program_name);

		elix_cstring_append(defaults, 255, "${binary_prefix}", 16);
		elix_cstring_append(defaults, 255, program_name, elix_cstring_length(program_name, 0));
		elix_cstring_append(defaults, 255, "${binary_suffix}${finalise_suffix} ", 35);
		
	} else {
		elix_file_write_string(&file, "\n", 1);
		elix_cstring_append(defaults, 255, "${binary_prefix}", 16);
		elix_cstring_append(defaults, 255, program_name, elix_cstring_length(program_name, 0));
		elix_cstring_append(defaults, 255, "${binary_suffix}${program_suffix} ", 35);

	}

	elix_file_write_formatted(&file, "default %s\n", defaults);
	elix_file_close(&file);


	for (size_t i = 0; i < ARRAY_SIZE(ini_list); i++){
		free(ini_list[i].buffer);
	}

	elix_file default_file;
	elix_file_open(&default_file, "build.ninja", EFF_FILE_WRITE);
	elix_file_write_formatted(&default_file, "include %s\n\n", filename);
	elix_file_close(&default_file);

	return 0;
}


uint32_t fg_config_arch(CompilerInfo * target, CurrentConfiguration *options, char * filename ){
	update_string_from_compilerinfo(config_arch_text, 512, target);

	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_WRITE);

	elix_file_write_string(&file, config_arch_text, 0);
	elix_file_close(&file);

	return 0;
}

uint32_t fg_config_default(CompilerInfo * target, CurrentConfiguration *options, char * filename ){

	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_WRITE);
	elix_file_write_string(&file, config_default_text, 0);
	elix_file_close(&file);
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
	elix_file_open(&file, filename, EFF_FILE_WRITE);

	elix_file_write_string(&file, config_platform_text[index].text, 0);
	elix_file_close(&file);


	return 0;
}

uint32_t fg_null(CompilerInfo * target, CurrentConfiguration *options, char * filename ){
	return 0;
}

uint32_t fg_defaultfilelist(CompilerInfo * target, CurrentConfiguration *options, char * filename ) {
	elix_directory * defaults_dir = elix_os_directory_list_files("./src/", nullptr);
	elix_path platfom_file = {0};

	if ( defaults_dir ) {
		elix_file file;
		elix_file_open(&file, filename, EFF_FILE_WRITE);

		for (size_t a = 0; a < defaults_dir->count; ++a) {
			elix_file_write_string(&file, defaults_dir->files[a].uri, 0);
			elix_file_write_string(&file, "\n", 1);
		}
		elix_file_close(&file);
		elix_os_directory_list_destroy(&defaults_dir);
	}
}
void creeate_newproject(CompilerInfo * target, CurrentConfiguration *options) {
	char directories_project[][64] = {
		"./config/",
		"./include/",
		"./res/",
		"./src/",
	};

	char file_generator[][128] = {
		"./config/modules/base.lst",
		"./config/default.txt",
	};

	uint32_t (*file_generator_function[])(CompilerInfo * target, CurrentConfiguration *options, char * filename ) = {
		&fg_defaultfilelist,
		&fg_config_default,
		&fg_null,
	};

	for (size_t i = 0; i < ARRAY_SIZE(directories_project); i++) {
		LOG_INFO("Creating Directory:\t%s", directories_project[i]);
		elix_os_directory_make(directories_project[i], 644, 1);
	}

	elix_file file;
	elix_file_open(&file, "./src/main.cpp", EFF_FILE_WRITE);
	elix_file_write_string(&file, "#include <iostream>\nint main(int argc, char *argv[]) {\n\tstd::cout << \"Hello World\" << std::endl;\n}", 0);
	elix_file_close(&file);


	for (size_t i = 0; i < ARRAY_SIZE(file_generator); i++) 	{
		elix_cstring_inreplace(file_generator[i], 128, "$platform", target->os);
		elix_cstring_inreplace(file_generator[i], 128, "$arch", target->arch);
		elix_cstring_inreplace(file_generator[i], 128, "$compiler", target->compiler);

		LOG_INFO("Creating File:\t%s", file_generator[i]);
		file_generator_function[i](target, options, file_generator[i]);
	}

}



void creeate_generator(CompilerInfo * target, CurrentConfiguration *options) {
	elix_file file_check;
	elix_file_open(&file_check, "./config/default.txt", EFF_FILE_READ_ONLY);
	if ( file_check.flag & EFF_FILE_READ_ERROR ) {
		LOG_INFO("Error: Run with the -platform argument");
		return;
	}
	elix_file_close(&file_check);


	char directories_generator[][128] = {
		"./build/",
		"./build/$platform-$arch",
	};

	char file_generator[][128] = {
		"./build/$platform-$arch/build.ninja",
	};

	uint32_t (*file_generator_function[])(CompilerInfo * target, CurrentConfiguration *options, char * filename ) = {
		&fg_build_ninja,
		&fg_null,
		&fg_null,
		&fg_null,
		&fg_null,
	};

	for (size_t i = 0; i < ARRAY_SIZE(directories_generator); i++) 	{
		elix_cstring_inreplace(directories_generator[i], 128, "$platform", target->os);
		elix_cstring_inreplace(directories_generator[i], 128, "$arch", target->arch);
		elix_cstring_inreplace(directories_generator[i], 128, "$compiler", target->compiler);

		LOG_INFO("Creating Directory:\t%s", directories_generator[i]);
		elix_os_directory_make(directories_generator[i], 755, 1);
	}

	for (size_t i = 0; i < ARRAY_SIZE(file_generator); i++) 	{
		elix_cstring_inreplace(file_generator[i], 128, "$platform", target->os);
		elix_cstring_inreplace(file_generator[i], 128, "$arch", target->arch);
		elix_cstring_inreplace(file_generator[i], 128, "$compiler", target->compiler);

		LOG_INFO("Writing File:\t%s", file_generator[i]);
		file_generator_function[i](target, options, file_generator[i]);
	}

}

void creeate_newplatform(CompilerInfo * target, CurrentConfiguration *options) {
	char directories_generator[][128] = {
		"./build/",
		"./build/$platform-$arch",
		"./config/",
		"./config/modules/",
	};

	char file_generator[][128] = {
		"./config/modules/base.lst",
		"./config/$platform-$arch.txt",
		"./config/$platform-common.txt",
		"./config/default.txt",
		"./build/$platform-$arch/build.ninja",
	};

	uint32_t (*file_generator_function[])(CompilerInfo * target, CurrentConfiguration *options, char * filename ) = {
		&fg_defaultfilelist,
		&fg_config_arch,
		&fg_config_platform,
		&fg_config_default,
		&fg_build_ninja,
		&fg_null,
		&fg_null,
		&fg_null,
		&fg_null,
	};

	for (size_t i = 0; i < ARRAY_SIZE(directories_generator); i++) 	{
		elix_cstring_inreplace(directories_generator[i], 128, "$platform", target->os);
		elix_cstring_inreplace(directories_generator[i], 128, "$arch", target->arch);
		elix_cstring_inreplace(directories_generator[i], 128, "$compiler", target->compiler);

		LOG_INFO("Creating Directory:\t%s", directories_generator[i]);
		elix_os_directory_make(directories_generator[i], 755, 1);
	}


	for (size_t i = 0; i < ARRAY_SIZE(file_generator); i++) 	{
		elix_cstring_inreplace(file_generator[i], 128, "$platform", target->os);
		elix_cstring_inreplace(file_generator[i], 128, "$arch", target->arch);
		elix_cstring_inreplace(file_generator[i], 128, "$compiler", target->compiler);

		LOG_INFO("Creating File:\t%s", file_generator[i]);
		file_generator_function[i](target, options, file_generator[i]);
	}


}

void bin_2_header( const char * source_file, const char * target_file, const char * variable_name ) {
	uint8_t c;
	size_t file_size = 0;
	elix_file source_stream, target_stream;

	elix_file_open( &target_stream, target_file, EFF_FILE_WRITE );
	elix_file_open( &source_stream, source_file, EFF_FILE_READ_ONLY );

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

int main(int argc, char * argv[]) {
	CurrentConfiguration configuration = {0};

	program_mode current_program_mode = PM_AUTO;
	uint8_t batch = 0;
	uint8_t release_mode = 0;
	uint8_t auto_gen = 0;

	int bin2h_index = 0;


	CompilerInfo info = check_preprocessor();

	for (uint8_t var = 1; var < argc; ++var) {
		//LOG_INFO("ARD[%d] %s", var, argv[var]);
		if ( elix_cstring_has_prefix(argv[var],"-help") ) {
			current_program_mode = PM_HELP;
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM=")) {
			elix_cstring_copy(argv[var]+9, info.os);
			find_plaform_details(&info);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_ARCH=")) {
			elix_cstring_copy(argv[var]+14, info.arch);
		} else if ( elix_cstring_has_prefix(argv[var],"PLATFORM_COMPILER=")) {
			elix_cstring_copy(argv[var]+18, info.compiler);
		} else if ( elix_cstring_has_prefix(argv[var],"TARGET_TRIPLET=")) {
			elix_cstring_copy(argv[var]+15, info.triplet);
		} else if ( elix_cstring_has_prefix(argv[var],"-batch") ) {
			batch = 1;
		} else if ( elix_cstring_has_prefix(argv[var],"-new") ) {
			current_program_mode = PM_NEWPROJECT;
		} else if ( elix_cstring_has_prefix(argv[var],"-platform") ) {
			current_program_mode = PM_NEWPLATFORM;
		} else if ( elix_cstring_has_prefix(argv[var],"-c=") ) {
			current_program_mode = PM_B2H;
			push_configmap(&configuration.options, argv[var], 0);
		} else {
			push_configmap(&configuration.options, argv[var], 0);
		}
	}

	if ( current_program_mode == PM_AUTO ) {
		char check_platformarch[128] = "./config/$platform-$arch.txt";

		elix_cstring_inreplace(check_platformarch, 128, "$platform", info.os);
		elix_cstring_inreplace(check_platformarch, 128, "$arch", info.arch);


		// If directory exist
		// current_program_mode = PM_NEWPROJECT;
		elix_file file_check;


		if ( !elix_file_open(&file_check, "./config/default.txt", EFF_FILE_READ_ONLY) ) {
			current_program_mode = PM_NEWPROJECT;
		}
		elix_file_close(&file_check);

		if ( current_program_mode == PM_AUTO ) {
			if ( !elix_file_open(&file_check, check_platformarch, EFF_FILE_READ_ONLY) ) {
				current_program_mode = PM_NEWPLATFORM;
			}
			elix_file_close(&file_check);
		}
		if ( current_program_mode == PM_AUTO )
			current_program_mode = PM_GEN;

	}


	switch (current_program_mode) {
		case PM_NEWPROJECT:
			LOG_INFO("Target OS: %s %s %x", info.os, info.arch, elix_hash(info.os, elix_cstring_length(info.os, 0)));
			LOG_INFO("Creating New Project");
			creeate_newproject(&info, &configuration);
			break;
		case PM_NEWPLATFORM:
			LOG_INFO("Target OS: %s %s %x", info.os, info.arch, elix_hash(info.os, elix_cstring_length(info.os, 0)));
			LOG_INFO("Generating Platform files");
			creeate_newplatform(&info, &configuration);
			break;
		case PM_GEN:
			LOG_INFO("Target OS: %s %s %x", info.os, info.arch, elix_hash(info.os, elix_cstring_length(info.os, 0)));
			LOG_INFO("Generating Build files");
			creeate_generator(&info, &configuration);
			break;
		case PM_B2H:
			LOG_INFO("binary to header");
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

					pid_t pid;
					int err = 0;

					#ifdef __WIN32__
					err = _spawnvp( _P_WAIT, arguments[0], arguments );
					#else
					err = posix_spawnp(&pid, arguments[0], nullptr, nullptr, arguments, nullptr );
					#endif

					if ( err != 0 ) {
						LOG_INFO("Can not convert binary to header");
					} else {
						LOG_INFO("Converted binary to header");
						LOG_INFO("Source: %s - Target: %s - Variable: %s", source_string,target_string, path.filename);
						bin_2_header( source_string, target_string, path.filename );
					}

				}

			} else {
				LOG_INFO("Can not convert binary to header");
			}
			break;
		default:
			print_help(&info);
			break;
	}

	return 0;
}
