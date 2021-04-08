/*************************************************************************
 * Generates build.ninja files for multiple platforms
 * build: gcc genscript.c -o genscript.exe
 * run: genscript.exe [-help]
 * 
 * 
*************************************************************************/

#include "genscript.h"


static char * ninja_extension_output[] = {
	"link_stage", "", "build ${binary_prefix}%s${binary_suffix}${program_suffix}: link %s\n",
	"final_stage", "", "build ${binary_prefix}%s${binary_suffix}${finalise_suffix}: finalise ${binary_prefix}%s${binary_suffix}${program_suffix}\n",
	"cpp", "o", "build ${object_dir}%s.%s: compile_cpp %s.cpp\n",
	"c", "o", "build ${object_dir}%s.%s: compile_c %s.c\n",
};

static char config_arch_text[512] = "[defines]\n\
PLATFORM_BITS=$bits\n\
[linker_flags]\n\
-Wl,-rpath -Wl,\\$$ORIGIN/lib\n\
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
clean=rm -rf ${object_dir}\n\
[commands-3ds]\n\
finalise=${finaliser} $in  $out  ${finalise_flags}\n\
\n\
";

	static char config_platform_text[512] = "[libs]\n\
stdc++\n\
m\n\
[lib_flags]\n\
-std=c++11\n\
\n\
[includes]\n\
[flags]\n\
-std=c++11\n\
[defines]\n\
PLATFORM_$PLATFORM\n\
";

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
	PM_HELP, PM_NEWPROJECT, PM_NEWPLATFORM, PM_GEN
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
				write_options = &options->defines;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[files]") ) {
				write_options = &options->files;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[lib_flags]") ) {
				write_options = &options->lib_flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[compiler_flags]") ) {
				write_options = &options->flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[final_flags]") ) {
				write_options = &options->final_flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[flags]") ) {
				write_options = &options->flags;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[libs]") ) {
				write_options = &options->libs;
				write_map = nullptr;
			} else if ( elix_cstring_has_prefix(data,"[options]") ) {
				write_options = nullptr;
				write_map = &options->options;
			} else if ( elix_cstring_has_prefix(data,platform_option) ) {
				write_options = nullptr;
				write_map = &options->options;
			} else if ( elix_cstring_has_prefix(data,"[commands]") ) {
				write_options = nullptr;
				write_map = &options->commands;
			} else if ( elix_cstring_has_prefix(data,commands_option) ) {
				write_options = nullptr;
				write_map = &options->commands;
			} else {
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

void creeate_newproject(CompilerInfo * target) {
	char directories_project[][64] = {
		"./include/",
		"./res/",
		"./src/",
	};


	for (size_t i = 0; i < ARRAY_SIZE(directories_project); i++) {
		LOG_INFO("Creating Directory:\t%s", directories_project[i]);
		elix_os_directory_make(directories_project[i], 644, 1);
	}

	elix_file file;
	elix_file_open(&file, "./src/main.cpp", EFF_FILE_WRITE);
	elix_file_write_string(&file, "#include <iostream>\nint main(int argc, char *argv[]) {\n\tstd::cout << \"Hello World\" << std::endl;\n}", 0);
	elix_file_close(&file);


}


uint32_t fg_build_ninja(CompilerInfo * target, CurrentConfiguration * options, char * filename ){
	LOG_INFO("Building %s", filename);
	
	JoinConfigList ini_list[] = {
		{ "-D%s ", &options->defines, nullptr},
		{ "%s ", &options->flags, nullptr},
		{ "-l%s ", &options->libs, nullptr},
		{ "%s ", &options->lib_flags, nullptr},
		{ "%s ", &options->final_flags, nullptr},
		{ "-i%s ", &options->includes, nullptr},
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


	char objects[6144] = {0};
	for (size_t i = 0; i < options->files.current; i++){
		for (size_t var = 0; var < ARRAY_SIZE(ninja_extension_output); var += 3) {
				if ( elix_cstring_has_suffix(options->files.value[i], ninja_extension_output[var]) ) {
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
					elix_file_write_formatted(&file, ninja_extension_output[var+2], no_src_dir, ninja_extension_output[var+1], options->files.value[i]);

					char format_buffer[128] = {0};
					size_t q = snprintf(format_buffer, 128, "${object_dir}%s.%s", no_src_dir, ninja_extension_output[var+1]);
					elix_cstring_append(objects, 6144, format_buffer, q);

					break;
				}
		}
		join_config_option(&ini_list[i]);
	}

	char * program_name = get_configmap(&options->options, "name");


	elix_file_write_formatted(&file, ninja_extension_output[2], program_name, objects);

	if ( find_configmap(&options->options, "finaliser") != SIZE_MAX ) {
		elix_file_write_string(&file, "\n", 1);
		elix_file_write_formatted(&file, ninja_extension_output[5], program_name, program_name);
		elix_file_write_formatted(&file, "default ${binary_prefix}%s${binary_suffix}${finalise_suffix}\n", program_name);
	} else {
		elix_file_write_string(&file, "\n", 1);
		elix_file_write_formatted(&file, "default ${binary_prefix}%s${binary_suffix}\n", program_name);
	}


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

	update_string_from_compilerinfo(config_platform_text, 512, target);

	elix_file file;
	elix_file_open(&file, filename, EFF_FILE_WRITE);

	elix_file_write_string(&file, config_platform_text, 0);
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


int main(int argc, char * argv[]) {
	CurrentConfiguration configuration = {0};

	program_mode current_program_mode = PM_GEN;
	uint8_t batch = 0;
	uint8_t release_mode = 0;
	uint8_t auto_gen = 0;


	CompilerInfo info = check_preprocessor();

	for (uint8_t var = 1; var < argc; ++var) {
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
		} else {
			push_configmap(&configuration.options, argv[var], 0);
		}
	}

	switch (current_program_mode) {
		case PM_NEWPROJECT:
			LOG_INFO("Target OS: %s %s %x", info.os, info.arch, elix_hash(info.os, elix_cstring_length(info.os, 0)));
			LOG_INFO("Creating New Project");
			creeate_newproject(&info);
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
		default:
			print_help(&info);
			break;
	}

	return 0;
}
