#ifndef ELIX_CSTRING_HPP
#define ELIX_CSTRING_HPP

#include "elix_core.h"

#ifndef SSIZE_MAX
#ifdef _WIN64
#define SSIZE_MAX INT64_MAX
#define SSIZE_MIN INT64_MIN
#else
#define SSIZE_MAX INT32_MAX
#define SSIZE_MIN INT32_MIN
#endif
#endif

inline uint32_t elix_hash( const char * str, size_t len ) {
	//Jenkins One-at-a-time hash
	uint32_t hash = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		hash += str[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}


inline size_t elix_cstring_length(const char * string, uint8_t include_terminator = 0 ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}

bool elix_cstring_has_suffix( const char * str, const char * suffix);
size_t elix_cstring_find_not_of( char * str, char * search, size_t offset = 0);
void  elix_cstring_sanitise( char * string );
char * elix_cstring_substr( const char * source, ssize_t pos = 0, ssize_t len = SSIZE_MAX );


#endif // ELIX_CSTRING_HPP
