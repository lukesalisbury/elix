#include "elix_os_window.hpp"

#if defined PLATFORM_WINDOWS
	#include "elix_os_window_win.cpp"
#elif defined PLATFORM_LINUX
	#include "elix_os_window_sdl2.cpp"
#elif defined PLATFORM_3DS

#elif defined PLATFORM_SDL2
	#include "elix_os_window_sdl2.cpp"	
#else
	#error "Unsupported platform"
#endif


