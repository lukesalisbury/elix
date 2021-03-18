#include "elix_os_window.hpp"

#if defined PLATFORM_WINDOWS
	#include "elix_os_window_win.cpp"
#elif defined PLATFORM_LINUX
	#include "elix_os_window_wayland.cpp"
#elif defined PLATFORM_3DS

#elif defined PLATFORM_SDL2

#else
	#error "Unsupported platform"
#endif
