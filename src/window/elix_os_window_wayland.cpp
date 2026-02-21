

#include "elix_os_window.hpp"

#ifdef ELIX_USE_WAYLAND
#include <wayland-egl.h>
#include <wayland-client.h>
#include <string.c>
#include "wayland/input-event-codes.h"
#include "wayland/xdg-shell-protocol.c"

/* Taken from the Weston Project */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int os_create_anonymous_file(size_t size) {
	const char *path;
	char name[256];
	int fd, ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		return -1;
	}

	if ( !snprintf(name, 255, "%s/elix-wayland-XXXXXX", path) ) {
		return -1;
	}

	fd = mkstemp(name);

	if (fd < 0) {
		return -1;
	}
	do {
		ret = ftruncate(fd, (off_t)size); //Note: Why use off_t ?
	} while (ret < 0);

	if (ret < 0) {
		close(fd);
		return -1;
	}

	return fd;
}
/* Taken from the Weston Project */


struct elix_os_window_wayland_client {
	wl_display * display = nullptr;
	wl_registry * registry = nullptr;

	wl_compositor * compositor = nullptr;
	wl_shm * shm = nullptr;
	xdg_wm_base * wm_base = nullptr;

	uint32_t window_counter = 0;
	uint32_t hi = 0;
};


static elix_os_window_wayland_client elix_os_wayland_client;


void elix_os_window_render(elix_os_window * w) {
	wl_surface_attach(w->surface, w->buffer, 0, 0);
	wl_surface_damage(w->surface, 0, 0, w->display_buffer->width, w->display_buffer->height);
	wl_surface_commit(w->surface);
}



static void elix_wayland_handle__surface_configure(void *data, struct xdg_surface * xdg_surface, uint32_t serial) {
	xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener elix_wayland_listener__xdg_surface = {
	elix_wayland_handle__surface_configure
};


static void elix_wayland_handle__toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states) {

}


static void elix_wayland_handle__toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {

}

static const struct xdg_toplevel_listener elix_wayland_listener__xdg_toplevel = {
	elix_wayland_handle__toplevel_configure,
	elix_wayland_handle__toplevel_close
};



static void elix_wayland_handle__pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
	elix_os_window * win = (elix_os_window*)data;
	LOG_MESSAGE("%d: pointer %x %d", time, button, state);
	if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
		//xdg_toplevel_move(win->xdg_toplevel, 0, serial);
	}
}

static void elix_wayland_handle__pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {

}

static void elix_wayland_handle__pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {

}

void elix_wayland_handle__pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y){

}

void elix_wayland_handle__pointer_axis( void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {

}

static const wl_pointer_listener elix_wayland_listener__pointer = {
	.enter = elix_wayland_handle__pointer_enter,
	.leave = elix_wayland_handle__pointer_leave,
	.motion = elix_wayland_handle__pointer_motion,
	.button = elix_wayland_handle__pointer_button,
	.axis = elix_wayland_handle__pointer_axis,
};

void elix_wayland_handle__keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size) {

}



void elix_wayland_handle__keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
	LOG_MESSAGE("keyboard_enter");
	elix_os_window * win = (elix_os_window*)data;

	elix_os_event_push_eventtype(win, EOE_WIN_ACTIVE);	
}

void elix_wayland_handle__keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface) {
	LOG_MESSAGE("keyboard_leave");
	elix_os_window * win = (elix_os_window*)data;

	elix_os_event_push_eventtype(win, EOE_WIN_INACTIVE);
}

void elix_wayland_handle__keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state){
	elix_os_window * win = (elix_os_window*)data;
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED && key == KEY_C && win->input_mods_depressed & 4) {
		LOG_MESSAGE("CTRL-C %p", win);
		win->flags = EOE_WIN_CLOSE;
		elix_os_event_push_eventtype(win, EOE_WIN_CLOSE);
	}

	
	if ( state == WL_KEYBOARD_KEY_STATE_PRESSED ) 
		elix_os_event_push_event(win, (elix_os_event){ EOE_INPUT_KEYDOWN, time, key });
	else if ( state == WL_KEYBOARD_KEY_STATE_RELEASED ) 
		elix_os_event_push_event(win, (elix_os_event){ EOE_INPUT_KEYUP, time, key });
}

void elix_wayland_handle__keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group){
	elix_os_window * win = (elix_os_window*)data;
	win->input_mods_depressed = mods_depressed;
	LOG_MESSAGE("Keyboard mod: %d %d %d %d", mods_depressed, mods_latched, mods_locked, group);
}

void elix_wayland_handle__keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay){
}

static const wl_keyboard_listener elix_wayland_listener__keyboard = {
	.keymap = elix_wayland_handle__keyboard_keymap,
	.enter = elix_wayland_handle__keyboard_enter,
	.leave = elix_wayland_handle__keyboard_leave,
	.key = elix_wayland_handle__keyboard_key,
	.modifiers = elix_wayland_handle__keyboard_modifiers,
	.repeat_info = elix_wayland_handle__keyboard_repeat_info
};

static void elix_wayland_handle__seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
	elix_os_window * win = (elix_os_window*)data;
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		struct wl_pointer *pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(pointer, &elix_wayland_listener__pointer, win);
	}
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		struct wl_keyboard * keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(keyboard, &elix_wayland_listener__keyboard, win);
	}
	if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
		struct wl_touch * touch = wl_seat_get_touch(seat);
		//wl_touch_add_listener(touch, &elix_wayland_listener__touch, win);
	}
}

static const struct wl_seat_listener elix_wayland_listener__seat = {
	.capabilities = elix_wayland_handle__seat_capabilities,
};



static void elix_wayland__registry_handler(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	elix_os_window * win = (elix_os_window*)data;
	//LOG_MESSAGE("%s", interface);
	if (strcmp(interface, "wl_compositor") == 0) {
		elix_os_wayland_client.compositor = (wl_compositor * )wl_registry_bind(elix_os_wayland_client.registry, id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_shm") == 0) {
		elix_os_wayland_client.shm = (wl_shm*)wl_registry_bind(registry,id, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		win->seat = (wl_seat *)wl_registry_bind(registry, id, &wl_seat_interface, 1);
		wl_seat_add_listener(win->seat, &elix_wayland_listener__seat, win);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		elix_os_wayland_client.wm_base = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
	} else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
		win->data_device_manager = (wl_data_device_manager*)wl_registry_bind(registry, id, &wl_data_device_manager_interface, 3);
	}
}

static void elix_wayland__registry_remover(void *data, wl_registry *registry, uint32_t id) {
	elix_os_window * win = (elix_os_window*)data;
	LOG_MESSAGE("Got a registry losing event for %d", id);
}

static const wl_registry_listener elix_wayland_listener__registry = {
	elix_wayland__registry_handler,
	elix_wayland__registry_remover
};




void elix_os_window_destroy( elix_os_window * win ) {
	elix_os_wayland_client.window_counter--;

	//wl_egl_window_destroy(win->window_handle);
	if (elix_os_wayland_client.window_counter == 0) {
		wl_display_disconnect(elix_os_wayland_client.display);
		printf("disconnected from display\n");
	}
}


wl_display * elix_os_window_display_get() {
	return elix_os_wayland_client.display;
}

wl_registry * elix_os_window_registry_get() {
	return elix_os_wayland_client.registry;
}



elix_os_window * elix_os_window_create(elix_uv32_2 dimension, elix_uv16_2 scale) {
	elix_os_wayland_client.window_counter++;

	elix_os_window * win = new elix_os_window;

	win->width = dimension.x;
	win->height = dimension.y;

	elix_os_wayland_client.display = wl_display_connect(nullptr);
	elix_os_wayland_client.registry = wl_display_get_registry( elix_os_wayland_client.display);

	wl_registry_add_listener(elix_os_wayland_client.registry, &elix_wayland_listener__registry, win);

	wl_display_roundtrip(elix_os_wayland_client.display);
	wl_registry_destroy(elix_os_wayland_client.registry);

	if (elix_os_wayland_client.compositor == nullptr) {
		LOG_MESSAGE("No wayland compositor founded");
		elix_os_wayland_client.window_counter--;
		return nullptr;
	}

	if (elix_os_wayland_client.shm == nullptr) {
		LOG_MESSAGE("No wayland shm founded");
		elix_os_wayland_client.window_counter--;
		return nullptr;
	}

	//Create Buffer
	size_t stride = win->width * 4;
	size_t size = stride * win->height;

	int fd = os_create_anonymous_file(size);
	if ( fd < 0) {
		LOG_MESSAGE("Shared Buffer not created");
		elix_os_wayland_client.window_counter--;
		return nullptr;
	}

	void * shm_buffer = nullptr;
	shm_buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (shm_buffer == MAP_FAILED) {
		LOG_MESSAGE("MMap failed");
		close(fd);
		elix_os_wayland_client.window_counter--;
		return nullptr;
	}

	win->display_buffer = new elix_graphic_data;

	win->display_buffer->width = dimension.width;
	win->display_buffer->height = dimension.height;
	win->display_buffer->bpp = 4;
	win->display_buffer->pixel_count = win->display_buffer->width * win->display_buffer->height;
	win->display_buffer->size = win->display_buffer->pixel_count * win->display_buffer->bpp;
	win->display_buffer->data = (uint8_t*)shm_buffer;



	wl_shm_pool * pool = wl_shm_create_pool(elix_os_wayland_client.shm, fd, size);
	win->buffer = wl_shm_pool_create_buffer(pool, 0, (int32_t)win->width, (int32_t)win->height, 4*win->width, WL_SHM_FORMAT_ARGB8888);

	wl_shm_pool_destroy(pool);
	close(fd);

	if (shm_buffer == nullptr) {
		LOG_MESSAGE("No shm buffer");
		elix_os_wayland_client.window_counter--;
		return nullptr;
	}

	win->surface = wl_compositor_create_surface( elix_os_wayland_client.compositor );

	win->xdg_surface = xdg_wm_base_get_xdg_surface( elix_os_wayland_client.wm_base, win->surface );
	win->xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);

	xdg_surface_add_listener(win->xdg_surface, &elix_wayland_listener__xdg_surface, win);
	xdg_toplevel_add_listener(win->xdg_toplevel, &elix_wayland_listener__xdg_toplevel, win);

	xdg_toplevel_set_title(win->xdg_toplevel, "Creationism");

	wl_display_roundtrip(elix_os_wayland_client.display);

	wl_surface_commit(win->surface);
	wl_display_roundtrip(elix_os_wayland_client.display);

	wl_surface_attach(win->surface, win->buffer, 0, 0);
	wl_surface_commit(win->surface);

	wl_display_flush(elix_os_wayland_client.display);
	return win;
}

bool elix_os_window_handle_events( elix_os_window * win) {
	win->flags = EOE_NONE;

	LOG_INFO("Window Events: %d", win->event_count);
	if ( wl_display_dispatch_pending(elix_os_wayland_client.display) >= 0 ){
		wl_display_dispatch(elix_os_wayland_client.display);
	}


	return true;
}



#include <wayland-client.h>
#include "elix_os_window.hpp"


struct elix_window_clipboard {
	
	wl_seat * seat = nullptr;
	char * text = nullptr;
	uint32_t current_serial = 0;
	elix_os_window * popup_window = nullptr;
};



static void elix_wayland_clipboard__popup_destroy(elix_window_clipboard * clipboard) {
	elix_os_window_destroy(clipboard->popup_window);
}
/*
static void elix_wayland_clipboard__registry_handler(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
	if ( !data ) {
		LOG_MESSAGE("No data");
		return;
	}

	elix_window_clipboard * clipboard = (elix_window_clipboard *)data;
	if (strcmp(interface, wl_seat_interface.name) == 0 && clipboard->seat == nullptr) {
		clipboard->seat = (wl_seat *)wl_registry_bind(registry, id, &wl_seat_interface, 1);
		//wl_seat_add_listener(clipboard->seat, &elix_wayland_listener__seat, clipboard);
	} else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
		clipboard->data_device_manager = (wl_data_device_manager*)wl_registry_bind(registry, id, &wl_data_device_manager_interface, 3);
	}

}


static const wl_registry_listener elix_wayland_listener__registry = {
	elix_wayland_clipboard__registry_handler
};
*/

char * test_str = "dfjroidlk";

static void data_source_handle_send(void * data, struct wl_data_source *source, const char *mime_type, int fd) {
	// An application wants to paste the clipboard contents
	elix_window_clipboard * clipboard = (elix_window_clipboard *)data;
	//LOG_MESSAGE("data_source_handle_send %s %d", mime_type, fd, clipboard->text);
	if (strcasecmp(mime_type, "text/plain") == 0) {
		write(fd, clipboard->text, strlen(clipboard->text));
	} else {
		fprintf(stderr,
			"Destination client requested unsupported MIME type: %s\n",
			mime_type);
	}
	close(fd);
}

static void data_source_handle_target(void* data, wl_data_source* source, const char* mimeType) {
	LOG_MESSAGE("data_source_handle_target %s", mimeType);
}

static void data_source_handle_cancelled(void *data, wl_data_source *source) {
	elix_window_clipboard * clipboard = (elix_window_clipboard *)data;
	LOG_MESSAGE("data_source_handle_cancelled %s", clipboard->text);

	wl_data_source_destroy(source);
	elix_os_window_destroy(clipboard->popup_window);
}



wl_display * elix_os_window_display_get();
wl_registry * elix_os_window_registry_get();

#include "elix_cstring.hpp"


void elix_clipboard_put(char * text) {
	elix_window_clipboard clipboard;

	struct wl_data_source_listener elix_clipboard_source_listener = {
		data_source_handle_target, //target
		data_source_handle_send, // send
		data_source_handle_cancelled, //cancelled
		nullptr, //dnd_drop_performed
		nullptr, //dnd_finished
		nullptr //action
	};
	clipboard.text = elix_cstring_copy(text);
	clipboard.popup_window = elix_os_window_create({1,1});
/*
	wl_display * display = wl_display_connect( nullptr );
	if (display == nullptr) {
		LOG_MESSAGE("failed to create display");
		return;
	}

	wl_registry * registry = wl_display_get_registry( display );
	if (registry == nullptr) {
		LOG_MESSAGE("failed to create registry");
		wl_display_disconnect(display);
		return;
	}

	wl_registry_add_listener(registry, &elix_wayland_listener__registry, &clipboard);
	wl_display_roundtrip(display);
	wl_display_dispatch(display);
*/
	wl_display_roundtrip(elix_os_window_display_get());
	wl_display_dispatch(elix_os_window_display_get());

	wl_data_device * data_device = wl_data_device_manager_get_data_device(clipboard.popup_window->data_device_manager, clipboard.popup_window->seat );
	wl_data_source * source = wl_data_device_manager_create_data_source(clipboard.popup_window->data_device_manager);
	wl_data_source_offer(source, "text/plain");
	wl_data_source_offer(source, "text/plain;charset=utf-8");
	wl_data_source_offer(source, "TEXT");
	wl_data_source_offer(source, "STRING");
	wl_data_source_offer(source, "UTF8_STRING");
	wl_data_source_add_listener(source, &elix_clipboard_source_listener, &clipboard);
	
	wl_data_device_set_selection(data_device, source, clipboard.current_serial);


	while (wl_display_dispatch(elix_os_window_display_get()) >= 0);

	elix_os_window_destroy(clipboard.popup_window);

}


#endif //ELIX_USE_WAYLAND
