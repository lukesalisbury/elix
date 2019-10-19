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

// Direcotry
bool elix_os_directory_make(const char * path, uint32_t mode = 744, bool parent = false, elix_consent * consent = nullptr);
bool elix_os_directory_remove(const char * path, bool forced = false, elix_consent * consent = nullptr);

uint8_t elix_os_directory_is(const char * path, elix_consent * consent = nullptr);

char ** elix_os_directory_list_files(const char * path, const char * suffix = nullptr, elix_consent * consent = nullptr);


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


#endif // ELIX_OS_BASE_HPP
