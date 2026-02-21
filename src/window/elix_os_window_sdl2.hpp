#ifndef ELIX_OSWINDOW_SDL2_HPP
#define ELIX_OSWINDOW_SDL2_HPP

#include "elix_core.h"

#include <SDL2/SDL.h>



struct elix_os_opengl {
	uint32_t version;
	//HGLRC context;
};



struct elix_os_window : elix_os_window_common {
	SDL_Window * window = nullptr;
	SDL_Renderer *renderer;
	SDL_Texture * texture;
	elix_os_opengl opengl_info;


};


#endif // ELIX_OSWINDOW_WAYLAND_HPP
