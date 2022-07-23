#ifdef PLATFORM

#include "elix_os_window.hpp"
/*
int8_t elix_os_opengl__setup_pixel_format(HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd;
	int pixelformat;

	memset( &pfd, 0, sizeof( pfd ) );

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.iPixelType = PFD_TYPE_COLORINDEX;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 16;
	pfd.cStencilBits = 8;

	if ( (pixelformat = ChoosePixelFormat(hdc, &pfd)) == 0 ) {
		return FALSE;
	}
	return !!SetPixelFormat(hdc, pixelformat, &pfd);
}

void elix_os_opengl_create(elix_os_window * win, uint32_t min_verison) {
	win->display_context = GetDC(win->window_handle);
	win->opengl_info.context = wglCreateContext(win->display_context);
	wglMakeCurrent(win->display_context, win->opengl_info.context);

//	printf("GL_VERSION: %n", glGetString(GL_VERSION));

}

void elix_os_opengl_destroy(elix_os_window * win) {
	if (win->opengl_info.context)
		wglDeleteContext(win->opengl_info.context);
}
*/


void elix_os_window__push_event( elix_os_window * win, uint32_t type ) {
	if( win->event_count < ARRAYCOUNT(win->events) ) {
		win->events[ win->event_count ].type = type;
		win->event_count++;
	}
}


void elix_os_window__render(elix_os_window * w) {
	BITMAPINFO bm = {
		{
			sizeof(tagBITMAPINFOHEADER),
			(long)w->display_buffer->width,
			(long)w->display_buffer->height,
			1,
			32,
			BI_RGB,
			0,
			96,
			96,
			0,
			0
		}, {{255,255,255,255}}
	};
	PAINTSTRUCT paint;
	RECT rect;
	BeginPaint(w->window_handle, &paint);
	GetClientRect(w->window_handle, &rect);

	if ( w->display_buffer->data ) {
		StretchDIBits(paint.hdc, 0, rect.bottom-1, rect.right, -rect.bottom, 0, 0, w->display_buffer->width, w->display_buffer->height, w->display_buffer->pixels, &bm, DIB_RGB_COLORS, SRCCOPY);
	} else {
		TextOut(paint.hdc, rect.bottom - 10 , 10, "No Image Buffer Found.", 22);
	}

	//TextOut(paint.hdc, 10, 10, "Hello", 5);
	EndPaint(w->window_handle, &paint);
}
void elix_os_window_render(elix_os_window * w) {
	InvalidateRect(w->window_handle, nullptr, true);
}


LRESULT CALLBACK elix_os_window__process_messages(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 1;
	elix_os_window * w = (elix_os_window*)(uintptr_t)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if ( !w ) return DefWindowProc(hwnd, msg, wParam, lParam);

	w->display_context = GetDC(w->window_handle);
	switch(msg)
	{
		case WM_QUIT:
			w->flags = EOE_QUIT | EOE_WIN_CLOSE;
			return 1;
		break;
		case WM_CLOSE:
			w->flags = EOE_WIN_CLOSE;
			return 1;
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
		case WM_SIZE:
		case WM_ACTIVATEAPP:
			InvalidateRect(hwnd, nullptr, true);
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
			elix_os_window__render(w);

		break;
		default:
			lRet = DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return lRet;
}


void elix_os_window_destroy( elix_os_window * win ) {
	if (win->display_context)
		ReleaseDC(win->window_handle, win->display_context);
	DestroyWindow(win->window_handle);

//	if (win->display_buffer.pixels)
//		delete [] win->display_buffer.pixels;
}

elix_os_window * elix_os_window_create(elix_uv32_2 dimension, elix_uv16_2 scale) {
	elix_os_window * win = new elix_os_window;

	win->instance_handle = GetModuleHandle( nullptr );
	win->width = dimension.width;
	win->height = dimension.height;
	win->dimension.width = dimension.width/scale.x;
	win->dimension.height = dimension.height/scale.y;
	win->display_buffer = elix_graphic_data_create({{dimension.width/scale.x, dimension.height/scale.y}});
	memset(win->display_buffer->pixels, 0xFFEEEEEE, win->display_buffer->pixel_count);

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_OWNDC, elix_os_window__process_messages, 0, 0,win->instance_handle,
					  nullptr,nullptr,nullptr,nullptr,"elixOsWindows", nullptr};

	if(!RegisterClassEx(&wc)) {
		MessageBox(nullptr, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
	}

	win->window_handle = CreateWindowExA(
		WS_EX_APPWINDOW|WS_EX_ACCEPTFILES,
		wc.lpszClassName,
		"Test Window",
		WS_OVERLAPPEDWINDOW,
		-1,
		-1,
		(int)win->width,
		(int)win->height,
		nullptr,
		nullptr,
		win->instance_handle,
		nullptr
	);

	if(win->window_handle == nullptr ) {
		MessageBox(nullptr, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
	} else {
		ShowWindow(win->window_handle, SW_SHOWNORMAL);
		UpdateWindow(win->window_handle);
	}

	SetWindowLongPtr(win->window_handle, GWLP_USERDATA, (LONG_PTR)win);

	win->display_context = GetDC(win->window_handle);
	return win;
}

bool elix_os_window_handle_events( elix_os_window * win) {
	MSG msg;
	win->flags = EOE_NONE;
	//InvalidateRect(win->window_handle, nullptr, true);
	while (PeekMessage(&msg, win->window_handle, 0, 0, PM_NOREMOVE))
	{
		if ( GetMessage(&msg, win->window_handle, 0, 0) ) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	SwitchToThread();
	return true;
}




char * elix_os_clipboard_get() {
	return nullptr;
}
void elix_os_clipboard_put(char * utfstring) {
	
}
