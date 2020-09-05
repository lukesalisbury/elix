#ifndef ELIX_OS_BASE_HPP
#define ELIX_OS_BASE_HPP

#include "elix_core.h"
#include "elix_consent.hpp"

struct elix_file_info {

};


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


// Directory


bool elix_os_directory_make(const char * path, uint32_t mode = 744, bool parent = false, elix_consent * consent = nullptr);
bool elix_os_directory_remove(const char * path, bool forced = false, elix_consent * consent = nullptr);

uint8_t elix_os_directory_is(const char * path, elix_consent * consent = nullptr);

char ** elix_os_directory_list_files(const char * path, const char * suffix = nullptr, elix_consent * consent = nullptr);
/*
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

void elix_os_file_association_make();
void elix_os_file_association_remove();

void elix_os_shortcut_desktop_make();
void elix_os_shortcut_desktop_remove();

void elix_os_shortcut_menu_make();
void elix_os_shortcut_menu_remove();

void elix_os_shortcut_startup_make();
void elix_os_shortcut_startup_remove();

// Stats/Info
struct elix_os_infomation {
	char os_name[8];
	char os_version[8];
	size_t available_processor = 1;
	size_t available_memory = 0;
	size_t available_vmemory = 0;
};
elix_os_infomation elix_os_infomation_get();
size_t elix_os_memory_usage();
size_t elix_os_vmemory_usage();
size_t elix_os_processor_usage();
size_t elix_os_vprocessor_usage();

void elix_os_system_idle(uint32_t time = 16000);
#endif // ELIX_OS_BASE_HPP
