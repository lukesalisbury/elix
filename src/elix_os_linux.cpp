#ifndef ELIX_OS_LINUX_HPP
#define ELIX_OS_LINUX_HPP

#include "elix_core.h"

#include <unistd.h>

size_t elix_os_memory_usage() {
	size_t rss = 0;
	FILE* fp = nullptr;
	if ( (fp = fopen( "/proc/self/statm", "r" )) != nullptr ) {
		fscanf( fp, "%*s%zd", &rss );
	}
	fclose( fp );
	return rss * static_cast<size_t>(sysconf( _SC_PAGESIZE));

}


#endif
