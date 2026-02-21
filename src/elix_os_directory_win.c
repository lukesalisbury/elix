#ifdef PLATFORM_WINDOWS

#include "elix_core.h"
#include "elix_consent.hpp"
#include "elix_cstring.hpp"
#include "elix_os.hpp"

#include <direct.h>
#include <errno.h>
#include <sys/stat.h>
/*
#include <fileapi.h>
#include <synchapi.h>
*/
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

#include <windows.h>

char * narrow_wchar(WCHAR * string, uint32_t length) {
	char * utf_string = nullptr;
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, string, (int)length, nullptr, 0, nullptr, nullptr);
	if ( size_needed > 1 ) {
		utf_string = new char[(size_t)size_needed+1]();
		WideCharToMultiByte(CP_UTF8, 0, string, (int)length, utf_string, size_needed, nullptr, nullptr);
		LOG_MESSAGE("%s %d", utf_string, size_needed);
	} else {
		LOG_MESSAGE("Invalid parameter;");
	}
	return utf_string;

}

/* Use OS to announce file changes
 * Note: may be reimplement later
bool elix_os_directory_watch_hander( elix_directory_watcher * watcher, uint32_t timeout, UNUSEDARG elix_consent * consent) {
	DWORD wait_status;
	bool is_active = true;
	DWORD BytesReturned;
	FILE_NOTIFY_INFORMATION buffer[1] = {};
	while (is_active) {
		LOGF_MESSAGE("Waiting %u seconds", timeout);
		wait_status = WaitForSingleObject(watcher->handle, timeout ? timeout: INFINITE);
		switch (wait_status) {
			case WAIT_OBJECT_0: {
				if ( FindNextChangeNotification(watcher->handle) != false ) {
					if ( ReadDirectoryChangesW(watcher->local_handle, &buffer, sizeof(buffer), TRUE,  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION, &BytesReturned, nullptr, nullptr) ) {
						char * filename = narrow_wchar(buffer[0].FileName, buffer[0].FileNameLength);
						ELIX_CHANGE_STATUS status = ELIX_CHANGE_UNKNOWN;
						switch (buffer[0].Action) {
							case FILE_ACTION_ADDED: {
								status = ELIX_CHANGE_FILE_ADDED;
								break;
							}
							case FILE_ACTION_REMOVED: {
								status = ELIX_CHANGE_FILE_REMOVED;
								break;
							}
							case FILE_ACTION_RENAMED_OLD_NAME: {

								break;
							}
							case FILE_ACTION_RENAMED_NEW_NAME: {

								break;
							}
							case FILE_ACTION_MODIFIED: {
								status = ELIX_CHANGE_FILE_UPDATE;
								break;
							}
						}

						if ( watcher->callback ) {
							watcher->callback(ELIX_CHANGE_UNKNOWN, watcher->directory, filename);
						} else {
							LOG_MESSAGE("File %d: %s %s", (int)status, watcher->directory, filename);
						}
						delete filename;
					}


				}
			}
			break;
			case WAIT_TIMEOUT:
				is_active = false;
				break;
			default:
				is_active = false;
		}
	}
	return is_active;
}

void elix_os_directory_watch_start(const char * path, elix_directory_watcher * watcher, UNUSEDARG elix_consent * consent) {
	ASSERT(watcher != nullptr)
	watcher->handle = FindFirstChangeNotification(path, false, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_DIR_NAME);
	watcher->local_handle = CreateFile(path,FILE_LIST_DIRECTORY,FILE_SHARE_DELETE|FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	watcher->directory = elix_cstring_from(path, "./");
}

void elix_os_directory_watch_stop( elix_directory_watcher * watcher, UNUSEDARG elix_consent * consent) {
	ASSERT(watcher != nullptr)
	FindCloseChangeNotification(watcher->handle);
}
*/

uint8_t elix_file_modified_check(const char * filename, int64_t * timestamp) {
	struct _stat64 file_info;

	if ( !_stat64( filename, &file_info ) ) {
		if ( !(file_info.st_mode & _S_IFREG)) {
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


#endif