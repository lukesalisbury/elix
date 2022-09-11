#ifndef ELIX_SYSTEMWINDOW_HPP
#define ELIX_SYSTEMWINDOW_HPP

#include "elix_core.h"
#include "elix_pixels.hpp"
#include "elix_events.hpp"


struct elix_os_window_common {
	elix_graphic_data * display_buffer;
	elix_uv32_2 position;
	elix_uv32_2 dimension;
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;
	elix_os_event_queue events;
	uint32_t flags;
};

struct elix_os_window_options {


};


struct elix_os_opengl;
struct elix_os_window;

#if defined PLATFORM_WINDOWS
	#include "elix_os_window_win.hpp"
#elif defined PLATFORM_LINUX
	//#include "elix_os_window_wayland.hpp"
	#include "elix_os_window_sdl2.hpp"
#elif defined PLATFORM_3DS
	#include "elix_os_window_3ds.hpp"
#else
	#error "Unsupported platform"
#endif

elix_os_window * elix_os_window_create( elix_uv32_2 dimension, elix_uv16_2 scale = {1,1}, const char * title = nullptr); //Todo: Add Extra options
bool elix_os_window_handle_events( elix_os_window * win );
void elix_os_window_render( elix_os_window * wwin );
void elix_os_window_destroy( elix_os_window * win );

char * elix_os_clipboard_get();
void elix_os_clipboard_put(const char * utfstring);

/*
void * elix_os_clipboard_get_data();
void elix_os_clipboard_put_data(char * utfstring);
*/

#endif // ELIX_SYSTEMWINDOW_HPP
