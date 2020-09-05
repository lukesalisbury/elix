#ifndef ELIX_OSWINDOW_WIN_HPP
#define ELIX_OSWINDOW_WIN_HPP

#include "elix_core.h"
#include <windows.h>
#include "elix_pixels.hpp"

struct elix_os_opengl {
	uint32_t version;
	HGLRC context;
};

struct elix_os_window : elix_os_window_common {
	HWND window_handle;
	HINSTANCE instance_handle;
	HDC display_context;
	elix_os_opengl opengl_info;
};


#endif // ELIX_OSWINDOW_WIN_HPP
