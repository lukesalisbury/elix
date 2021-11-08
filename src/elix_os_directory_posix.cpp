
#include "elix_core.h"
#include "elix_consent.hpp"
#include "elix_cstring.hpp"

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

bool elix_os_directory_make(const char * path, uint32_t mode, UNUSEDARG bool parent, UNUSEDARG elix_consent * consent) {
	//TODO Check consent

	if ( mode < 100 ) {
		//TODO: Better checks for directory mode
		LOGF_ERROR("Directory '%s', will nmot be readable", path);
	}

	int32_t err = mkdir(path, mode);
	if ( !err ) {
		return true;
	}


	if ( err == EEXIST ) {
		LOG_MESSAGE("Directory was not created because dirname is the name of an existing file, directory, or device. %s", path);
	} else if ( err == EACCES ) {
		LOG_MESSAGE("Search permission is denied on a component of the path prefix, or write permission is denied on the parent directory of the directory to be created. %s", path);
	}


	return false;
}

bool elix_os_directory_remove(const char * path, UNUSEDARG bool forced, UNUSEDARG elix_consent * consent) {
	//TODO Check consent

	int32_t err = rmdir(path);
	if ( !err ) {
		return true;
	}

	if ( err == ENOTEMPTY ) {
		LOG_MESSAGE("Given path is not a directory, the directory is not empty, or the directory is either the current working directory or the root directory. %s", path);
	} else if ( err == EACCES ) {
		LOG_MESSAGE("Search permission is denied on a component of the path prefix, or write permission is denied on the parent directory of the directory to be removed. %s", path);
	}

	return false;

}


uint8_t elix_os_directory_is( const char * path, UNUSEDARG elix_consent * consent){
	//TODO Check consent
	struct stat64 directory;

	int32_t err = stat64( path, &directory );
	if ( !err ) {
		return S_ISDIR(directory.st_mode);
	}

	if ( err == ENOENT ) {
		//LOG_MESSAGE("Path was not found. %s", path);
	} else if ( err == EINVAL  ) {
		LOG_MESSAGE("Invalid parameter; . %s", path);
	}
	return false;
}


elix_directory * elix_os_directory_list_files(const char * path, const char * suffix, elix_consent * consent) {
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