#include "elix_os.h"

#ifdef ELIX_USE_DIRECTORY_POSIX


#include "elix_consent.h"
#include "elix_cstring.h"

#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

bool elix_os_directory_make(const char * path, uint32_t mode, UNUSEDARG bool parent, UNUSEDARG elix_consent * consent) {
	//TODO: Check consent
	if ( mode < 100 ) {
		//TODO: Better checks for directory mode
		LOGF_ERROR("Directory '%s', will not be readable", path);
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
	//TODO: Check consent
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

void elix_os_directory_list_destroy(elix_directory ** directory) {
	for (int f = 0; f < (*directory)->count; ++f) {
		NULLIFY((*directory)->files[f].uri)
		NULLIFY((*directory)->files[f].path)
		NULLIFY((*directory)->files[f].filename)
		NULLIFY((*directory)->files[f].filetype)
	}
	(*directory)->count = 0;
	NULLIFY((*directory)->files)
	//NULLIFY(*directory)
}


elix_directory * elix_os_directory_list_create(const char * path, const char * suffix, elix_consent * consent) {
	elix_directory * directory = nullptr;
	DIR * current_directory = nullptr;
	struct dirent * entity = nullptr;
	
	uint16_t index = 0;
	size_t path_len = elix_cstring_length(path, 0);
	char buffer[ELIX_FILE_PATH_LENGTH] = {0};

	current_directory = opendir(path);
	if ( !current_directory ) {
		return directory;
	}

	directory = (elix_directory*)calloc(1, sizeof(elix_directory));
	while (entity = readdir(current_directory) ) {
		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)){

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) )
				directory->count++;
		} else {
			directory->count++;
		}
	}
	
	rewinddir(current_directory);


	directory->files = ALLOCATE(elix_path, directory->count);
	
	elix_cstring_copy(path, buffer, ELIX_FILE_PATH_LENGTH);

	//Append / if missing
	if ( !elix_cstring_has_suffix(buffer, "/") ) {
		elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, "/", 1);
		path_len++;
	}

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


uint8_t elix_file_modified_check(const char * filename, int64_t * timestamp) {
	struct stat64 file_info;
	
	if ( !stat64( filename, &file_info ) ) {
		if ( !(file_info.st_mode & S_IFREG)) {
			*timestamp = -1;
			return 0; // NOT_A_FILE
		}

		if ( *timestamp < (int64_t)file_info.st_mtime ) {
			*timestamp = (int64_t)file_info.st_mtime;
			return 2; //FILE_MODIFED
		}
		return 1; //FILE_UNCHANGED
	}
	*timestamp = -1;
	return -1; // FILE_REMOVED

}


#endif //ELIX_USE_DIRECTORY_POSIX
