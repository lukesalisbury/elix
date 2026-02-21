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


enum ELIX_DIRECTORY_ACCESS {
	ELIX_DIR_FAILED,
	ELIX_DIR_R,
	ELIX_DIR_RW
};

enum ELIX_CHANGE_STATUS {
	ELIX_CHANGE_FILE_UPDATE,
	ELIX_CHANGE_FILE_ADDED,
	ELIX_CHANGE_FILE_REMOVED,
	ELIX_CHANGE_DIRECTORY_UPDATE,
	ELIX_CHANGE_DIRECTORY_ADDED,
	ELIX_CHANGE_DIRECTORY_REMOVED,
	ELIX_CHANGE_UNKNOWN
};

typedef enum ELIX_PROGRAM_DIRECTORY {
	EPDS_DOCS_PUBLIC,
	EPDS_DOCS_PRIVATE,
	EPDS_USER_ROAMING,
	EPDS_USER_LOCAL,
	ELIX_PROGRAM_DIRECTORY_LENGTH
} ELIX_PROGRAM_DIRECTORY;

struct elix_file_info {

};

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

/*
struct elix_path_uri {
	char * scheme = nullptr;
	char * host = nullptr;
	uint16_t port = 0;
	char * path = nullptr;
	char * filename = nullptr;
	char * query = nullptr;
	char * fragment = nullptr;
	char * user = nullptr;
	char * password = nullptr;
};
*/

enum elix_program_resource_directory {
	EPRD_AUTO,
	EPRD_SHARE, // ./share/"executable"/
	EPRD_SHARE_IN_PARENT, // ../share/"executable"/ 
	EPRD_DATA, // ./"executable"_data/
	EPRD_GLOBAL, // /usr/share/"program_directory"/
};

typedef struct elix_program_info {
	char user[16];
	char * program_name;
	char * program_version;
	char * program_version_level;
	char * program_directory;
	elix_path path_executable;
	uint32_t resource_directory_type;
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
void elix_path_destroy(elix_path * directory);

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

// Associations/Shortcuts/b
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
elix_program_info elix_program_info_create(const char * arg0, const char * program_name, const char * version, const char * version_level );

char * elix_program_directory_documents(const elix_program_info * program_info, bool shared, const char * filename );
char * elix_program_directory_user( const elix_program_info * program_info, bool roaming, const char * filename );
char * elix_program_directory_resources(const elix_program_info * program_info, const char * filename, uint8_t override_lookup);
char * elix_program_directory_cache_file( const elix_program_info * program_info, const char * filename);

char * elix_program_directory_root( const elix_program_info * program_info, const char * filename);


#ifdef __cplusplus
}
#endif

inline bool elix_os_directory_make_stupid_c(const char * path) {
	return elix_os_directory_make(path, 744, false, nullptr);
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
