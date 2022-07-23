#include "elix_os_window.hpp"
#include "elix_os_window_sdl2.hpp"

char * elix_os_clipboard_get() {
	return SDL_GetClipboardText();
}

void elix_os_clipboard_put(char * utfstring) {
	LOG_INFO("Writing to clipboard '%s'", utfstring );
	SDL_SetClipboardText(utfstring);
}


elix_os_window * elix_os_window_create( elix_uv32_2 dimension, elix_uv16_2 scale, const char * title) {

	elix_os_window * win = new elix_os_window;

	uint32_t flags = SDL_WINDOW_SHOWN;

	SDL_Init(SDL_INIT_VIDEO);
	win->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dimension.width, dimension.height, flags);
	win->renderer = SDL_CreateRenderer(win->window,-1,SDL_RENDERER_ACCELERATED );

	win->width = dimension.width;
	win->height = dimension.height;
	win->dimension.width = dimension.width/scale.x;
	win->dimension.height = dimension.height/scale.y;

	win->texture = SDL_CreateTexture(win->renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING, win->dimension.width, win->dimension.height);

	win->display_buffer = elix_graphic_data_create({{win->dimension.width, win->dimension.height}});
	memset(win->display_buffer->pixels, 0xFFEEEEEE, win->display_buffer->pixel_count);

	return win;

}

bool elix_os_window_handle_events( elix_os_window * win ) {
	SDL_Event event;
	SDL_GameController * controller = nullptr;

	while (SDL_PollEvent(&event)) {
		switch( event.type ) {
				case SDL_TEXTINPUT: {
					break;
				}
				case SDL_MOUSEMOTION: {

					break;
				}
				case SDL_MOUSEWHEEL: {
					if ( event.wheel.which != SDL_TOUCH_MOUSEID ) {
						
					}
					break;
				}
				case SDL_QUIT: {
					elix_os_event_push_eventtype(&win->events, EOE_WIN_CLOSE);
					win->flags = EOE_WIN_CLOSE;
					break;
				}
				case SDL_WINDOWEVENT: {
					if ( event.window.windowID == SDL_GetWindowID( win->window ) ) {
						
						if ( event.window.event == SDL_WINDOWEVENT_MINIMIZED ) {

						} else if ( event.window.event == SDL_WINDOWEVENT_RESTORED ) {

						} else if ( event.window.event == SDL_WINDOWEVENT_MAXIMIZED ) {

						} else if ( event.window.event == SDL_WINDOWEVENT_ENTER ) {

						} else if ( event.window.event == SDL_WINDOWEVENT_LEAVE )	{

						} else if ( event.window.event == SDL_WINDOWEVENT_RESIZED ) {
							//SDL_RenderSetLogicalSize(app->renderer, event.window.data1, event.window.data2);
							//SDL_RenderSetScale(app->renderer,  (float)event.window.data1 / (float)app->viewpoint.w, (float)event.window.data2 / (float)app->viewpoint.h);
						} else if ( event.window.event == SDL_WINDOWEVENT_CLOSE ) {
							elix_os_event_push_eventtype(&win->events, EOE_WIN_CLOSE);
							win->flags = EOE_WIN_CLOSE;
						}
						
					}
					break;
				}
				case SDL_CONTROLLERDEVICEADDED:
					if ( SDL_IsGameController(event.cdevice.which) ) {
						controller = SDL_GameControllerOpen(event.cdevice.which);
						//TODO: Better handling then this. as which refers to joystick not gamepad id
						const char * name = SDL_GameControllerName(controller);
						if ( event.cdevice.which >= 0 && event.cdevice.which < 4) {
							//TODO:Better handing of controller name
						}
					}
					break;
				case SDL_CONTROLLERDEVICEREMOVED:
					controller = SDL_GameControllerFromInstanceID(event.cdevice.which);
					SDL_GameControllerClose(controller);
					break;
				case SDL_CONTROLLERAXISMOTION:
					controller = SDL_GameControllerFromInstanceID(event.caxis.which);
					break;
				case SDL_CONTROLLERBUTTONDOWN:
					controller = SDL_GameControllerFromInstanceID(event.cbutton.which);
					break;
				case SDL_CONTROLLERBUTTONUP:
					controller = SDL_GameControllerFromInstanceID(event.cbutton.which);
					break;
				case SDL_USEREVENT:

					break;
				case SDL_KEYDOWN:
				{
					switch ( event.key.keysym.sym )
					{
						case SDLK_ESCAPE:
						case SDLK_AC_BACK:
							break;
						case SDLK_PAUSE:
							break;
						case SDLK_F4: //
						{
							if ( (event.key.keysym.mod & KMOD_ALT) )
							{
							}
							break;
						}

						case SDLK_F5: // Quick Save
						{
							break;
						}
						case SDLK_F6: // Quick Load
						{
							break;
						}
						case SDLK_SYSREQ:
						{
							break;
						}
						case SDLK_BACKSPACE:
						{
							event.key.keysym.sym = 8; //OS X had issue - Don't know if it still does in SDL2
							break;
						}
						case SDLK_DELETE:
						{
							event.key.keysym.sym = 127;
							break;
						}
						case SDLK_RETURN:
						{
							/* Full Screen */
							if ( (event.key.keysym.mod & KMOD_ALT) )
							{
								Uint32 flags = SDL_GetWindowFlags(win->window);
								SDL_SetWindowFullscreen(win->window, !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP));
							}
							break;
						}
						case SDLK_v:
						{
							/* Paste Text */
							if ( (event.key.keysym.mod & KMOD_CTRL) && SDL_HasClipboardText() )
							{

							}
							break;
						}

					}
					break;
				}
				default:
					break;
			}
	}
	return true;
}

void elix_os_window_render( elix_os_window * win ) {
	SDL_RenderClear(win->renderer);
	SDL_UpdateTexture(win->texture, nullptr, win->display_buffer->pixels, win->display_buffer->width * win->display_buffer->bpp);
	SDL_RenderCopy(win->renderer, win->texture, nullptr, nullptr);
	SDL_RenderPresent(win->renderer);
}
void elix_os_window_destroy( elix_os_window * win ) {
	SDL_DestroyRenderer(win->renderer);
	SDL_DestroyWindow(win->window);
}