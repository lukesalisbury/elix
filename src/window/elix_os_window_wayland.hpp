#ifndef ELIX_OS_WINDOW_WAYLAND_HPP
#define ELIX_OS_WINDOW_WAYLAND_HPP

#ifdef ELIX_USE_WAYLAND

#include "elix_core.h"

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include "xdg-shell-client-protocol.h"

#define wl_array_for_each_type(pos, array, type) for (pos = (type)(array)->data;(const char *) pos < ((const char *) (array)->data + (array)->size); (pos)++)

union nint32_t {
	uint32_t u;
	int32_t s;
};


struct elix_os_opengl {
	uint32_t version;
	//HGLRC context;
};



struct elix_os_window : elix_os_window_common {

	/* local */
	wl_data_device_manager * data_device_manager = nullptr;
	wl_buffer * buffer;
	wl_shm_pool * pool;
	wl_surface * surface;
	struct xdg_surface *  xdg_surface;
	struct xdg_toplevel * xdg_toplevel;
	wl_seat * seat;

	uint32_t input_mods_depressed;


};

#endif // ELIX_USE_WAYLAND
#endif // ELIX_OS_WINDOW_WAYLAND_HPP
