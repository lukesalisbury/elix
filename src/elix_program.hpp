#ifndef ELIX_PROGRAM_HPP
#define ELIX_PROGRAM_HPP

#include <string.h>
#include "elix_core.h"

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

enum elix_program_resource_directory {
	EPRD_AUTO,
	EPRD_SHARE,
	EPRD_SHARE_IN_PARENT,
	EPRD_DATA
};

struct elix_program_info {
	char * user;
	char * program_name;
	char * program_version;
	char * program_version_level;
	char * program_directory;
	elix_path path_executable;
	uint32_t resource_directory_type = EPRD_AUTO;
};

elix_program_info elix_program_info_create(const char * arg0, const char * program_name, const char * version, const char * version_level = nullptr );

char * elix_program_directory_documents(const elix_program_info * program_info, bool shared, const char * filename = nullptr );
char * elix_program_directory_user( const elix_program_info * program_info, bool roaming = false, const char * filename = nullptr );
char * elix_program_directory_resources(const elix_program_info * program_info, const char * filename = nullptr , uint8_t override_lookup = EPRD_AUTO);
char * elix_program_directory_cache_file( const elix_program_info * program_info, const char * filename);

#endif // ELIX_PROGRAM_HPP
