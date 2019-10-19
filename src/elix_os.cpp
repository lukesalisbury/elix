#include "elix_os.hpp"
#include "elix_cstring.hpp"
#include <string.h>

char * elix_cstring_substr( const char * source, ssize_t pos, ssize_t len ) {
	ASSERT(source != nullptr)

	size_t offset = 0;
	size_t max_length = elix_cstring_length(source);
	size_t length = max_length;

	if ( pos < 0 ) {
		offset = length + pos;
	} else {
		offset = pos;
	}

	if ( len != SSIZE_MAX && len != SSIZE_MIN) {
		if ( len < 0 ) {
			length += len;
		} else if ( (size_t)len < length ) {
			length = (size_t)len + offset;
		}
	}

	length -= offset;
	if ( length + offset > max_length ){
		LOG_ERROR("elix_cstring_substr failed, %zu + %zu > %zu", length, offset, max_length);
		return nullptr;
	}

	if ( length < 1 || length > max_length) {
		LOG_ERROR("elix_cstring_substr failed, Length is invalid (%zu).", length);
		return nullptr;
	}

	char * dest = new char[length+1]();
	memcpy(dest, source + offset, length);

	return dest;
}


#if defined PLATFORM_WINDOWS
	#include "elix_os_directory_win.cpp"
	#include "elix_os_win.cpp"
#elif defined PLATFORM_LINUX
	#include "elix_os_directory_posix.cpp"
	#include "elix_os_linux.cpp"
#elif defined PLATFORM_3DS

#elif defined PLATFORM_NXSWITCH

#elif defined PLATFORM_SDL2

#else
	#error "Unsupported platform"
#endif
