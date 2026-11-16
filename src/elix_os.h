#ifndef ELIX_OS_BASE_HPP
#define ELIX_OS_BASE_HPP

#if defined PLATFORM_WINDOWS

#elif defined PLATFORM_LINUX
	#define ELIX_USE_DIRECTORY_POSIX 1
#elif defined PLATFORM_3DS
	#define ELIX_USE_DIRECTORY_POSIX 1
#elif defined PLATFORM_SDL2
	#define ELIX_USE_DIRECTORY_POSIX 1
#elif defined PLATFORM_SDL3
	//#include "elix_os_directory_sdl3.cpp"
#else
	#error "Unsupported platform"
#endif

#include "elix_core.h"
#include "elix_consent.h"

enum ELIX_CHANGE_STATUS {
	ELIX_CHANGE_FILE_UPDATE,
	ELIX_CHANGE_FILE_ADDED,
	ELIX_CHANGE_FILE_REMOVED,
	ELIX_CHANGE_DIRECTORY_UPDATE,
	ELIX_CHANGE_DIRECTORY_ADDED,
	ELIX_CHANGE_DIRECTORY_REMOVED,
	ELIX_CHANGE_UNKNOWN
};

typedef enum elix_program_special_directory {
	EPSD_SYSTEM_CONFIG, // XDG_CONFIG_DIRS
	EPSD_SYSTEM_RESOURCE, // XDG_DATA_DIRS or CSIDL_COMMON_DOCUMENTS or /usr/share/%program%/
	EPSD_USER_CONFIG, // XDG_CONFIG_HOME or CSIDL_APPDATA
	EPSD_USER_RESOURCE, // XDG_DATA_HOME or CSIDL_APPDATA
	EPSD_USER_DOCS, // ~/Documents or CSIDL_PERSONAL
	EPSD_USER_CACHE, // XDG_CACHE_HOME or CSIDL_LOCAL_APPDATA
	EPSD_USER_TEMP, // XDG_RUNTIME_DIR
	ELIX_PROGRAM_SPECIAL_DIRECTORY_LENGTH
} elix_program_special_directory;

typedef enum elix_filesystem_access {
	ESA_INVALID,
	ESA_DIR_EXISTS,
	ESA_DIR_R,
	ESA_DIR_RW,
	ESA_FILE_EXISTS,
	ESA_FILE_R,
	ESA_FILE_RW,
	ESA_FILE_EXE,
} elix_filesystem_access;

typedef enum elix_program_resource_directory {
	EPRD_AUTO,
	EPRD_GLOBAL_SHARE, 		// /usr/share/"program_name"/
	EPRD_LOCAL_SHARE, 		// ~/.local/share/"program_name"/
	EPRD_PARENT_SHARE, 	// ../share/"program_name"/ 
	EPRD_SHARE,			// ./share/"program_name"/
	EPRD_DATA,			// ./"program_name"_data/
	//MUTATABLE options allow the program to be rename to have seperate resources
	//No EPRD_GLOBAL_MUTATABLE or EPRD_LOCAL_SHARE as it would allow reading of other programs files
	EPRD_PARENT_SHARE_MUTATABLE, 	// ../share/"executable"/
	EPRD_SHARE_MUTATABLE, 			// ./share/"executable"/
	EPRD_DATA_MUTATABLE, 			// ./"executable"_data/
} elix_program_resource_directory;

typedef struct elix_path {
	char * uri;
	char * path;
	char * filename;
	char * filetype;
} elix_path;

typedef struct elix_directory {
	uint16_t count;
	elix_path * files;
} elix_directory;

typedef struct elix_program_info {
	char system_user[16];
	char program_name[32];
	char program_version[8];
	char program_version_directory[16];
	char program_directory[40];
	char program_executable[32];
	elix_path path_executable;
} elix_program_info;

typedef struct elix_os_infomation {
	char os_name[8];
	char os_version[8];
	size_t available_processor;
	size_t available_memory;
	size_t available_vmemory;
} elix_os_infomation;

#ifdef __cplusplus
extern "C" {
#endif


// Path
elix_path elix_path_create(const char * string);
void elix_path_destroy(elix_path * path);

elix_filesystem_access elix_os_path_status( const char * path );

// Directory
bool elix_os_directory_make(const char * path, uint32_t mode, bool parent, elix_consent * consent);
bool elix_os_directory_remove(const char * path, bool forced, elix_consent * consent);
uint8_t elix_os_directory_is(const char * path, elix_consent * consent);

elix_directory * elix_os_directory_list_create(const char * path, const char * suffix, elix_consent * consent);
void elix_os_directory_list_destroy(elix_directory ** directory);

uint8_t elix_file_modified_check(const char * filename, int64_t * timestamp);


/* Still abit buggy
struct elix_directory_watcher {
	void (*callback)(ELIX_CHANGE_STATUS status, char * directory, MUSTFREEARG char * filename);
	void * handle;
	void * local_handle;
	char * directory;
};
void elix_os_directory_watch_start(const char * path, elix_directory_watcher * watcher, elix_consent * consent = nullptr);
void elix_os_directory_watch_stop( elix_directory_watcher * watcher, elix_consent * consent = nullptr);
*/

// Font
elix_databuffer elix_os_font( const char * font_name );

// Time
// date
// Threads/Fibers/Workers

// Network
void elix_os_socket_create();
void elix_os_socket_destroy();

// Associations/Shortcuts
/*
void elix_os_file_association_make();
void elix_os_file_association_remove();

void elix_os_shortcut_desktop_make();
void elix_os_shortcut_desktop_remove();

void elix_os_shortcut_menu_make();
void elix_os_shortcut_menu_remove();

void elix_os_shortcut_startup_make();
void elix_os_shortcut_startup_remove();
*/

// Stats/Info

elix_os_infomation elix_os_infomation_get();
size_t elix_os_memory_usage();
size_t elix_os_vmemory_usage();
size_t elix_os_processor_usage();
size_t elix_os_vprocessor_usage();

void elix_os_system_idle(uint32_t time);

//Program

elix_program_info elix_program_info_create(const char * arg0, const char * program_name, const char * version_release, /*NULL*/ const char * version_directory);
void elix_program_info_set_resource( elix_program_info * info, elix_program_resource_directory source);

char * elix_program_directory_config( const elix_program_info * program_info, const char * filename, uint8_t always_return, uint8_t system_version );
char * elix_program_directory_resource( const elix_program_info * program_info, const char * filename, uint8_t always_return, elix_program_resource_directory resource_directory );
char * elix_program_directory_document( const elix_program_info * program_info, const char * filename, uint8_t always_return, uint8_t request_root_directory );
char * elix_program_directory_cache( const elix_program_info * program_info, const char * filename, uint8_t always_return, uint8_t system_version );


#ifdef __cplusplus
}
#endif

inline bool elix_os_directory_make_stupid_c(const char * path) {
	return elix_os_directory_make(path, 0755, false, nullptr);
}
inline bool elix_os_directory_remove_stupid_c(const char * path) {
	return elix_os_directory_remove(path, false, nullptr);
}
inline uint8_t elix_os_directory_is_stupid_c(const char * path) {
	return elix_os_directory_is(path, nullptr);
}
inline elix_directory * elix_os_directory_list_create_stupid_c(const char * path) {
	return elix_os_directory_list_create(path, nullptr, nullptr);
}

#endif // ELIX_OS_BASE_HPP
