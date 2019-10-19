
#include "elix_core.h"
#include "elix_consent.hpp"
#include "elix_cstring.hpp"

#include <direct.h>
#include <errno.h>
#include <sys/stat.h>

bool elix_os_directory_make(const char * path, uint32_t mode, UNUSEDARG bool parent, UNUSEDARG elix_consent * consent) {
	//TODO Check consent

	if ( mode != 744 ) {
		LOGF_ERROR("On Windows, mode is ignored");
	}

	if ( !_mkdir(path) ) {
		return true;
	}

	errno_t err;
	_get_errno( &err );

	if ( err == EEXIST ) {
		LOG_MESSAGE("Directory was not created because dirname is the name of an existing file, directory, or device. %s", path);
	} else if ( err == ENOENT ) {
		LOG_MESSAGE("Path was not found. %s", path);
	}

	return false;
}

bool elix_os_directory_remove(const char * path, UNUSEDARG bool forced, UNUSEDARG elix_consent * consent) {
	//TODO Check consent

	if ( !_rmdir(path) ) {
		return true;
	}
	errno_t err;
	_get_errno( &err );

	if ( err == ENOTEMPTY ) {
		LOG_MESSAGE("Given path is not a directory, the directory is not empty, or the directory is either the current working directory or the root directory. %s", path);
	} else if ( err == ENOENT ) {
		LOG_MESSAGE("Path was not found. %s", path);
	} else if ( err == EACCES ) {
		LOG_MESSAGE("A program has an open handle to the directory.. %s", path);
	}

	return false;

}

uint8_t elix_os_directory_is( const char * path, UNUSEDARG elix_consent * consent){
	//TODO Check consent

	if ( elix_cstring_has_suffix(path, "/")) {
		uint8_t suberr = 0;
		char * clean_path = nullptr;
		clean_path = elix_cstring_substr(path,0, -1);
		if ( clean_path ) {
			suberr = elix_os_directory_is(clean_path, consent);
			delete clean_path;
		}
		return suberr;
	}

	struct _stat64 directory;

	if ( !_stat64( path, &directory ) ) {
		return !!(directory.st_mode & _S_IFDIR);
	}

	errno_t err;
	_get_errno( &err );
	if ( err == ENOENT ) {
		//LOG_MESSAGE("Path was not found. %s", path);
	} else if ( err == EINVAL  ) {
		LOG_MESSAGE("Invalid parameter; . %s", path);
	}
	return false;
}

