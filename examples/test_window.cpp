#include "elix_endian.h"
#include "elix_html.h"

#include "extra/elix_fpscounter.hpp"
#include "window/elix_os_window.hpp"

#include "elix_rgbabuffer.h"
#include "elix_cstring.h"
#include "elix_os.h"
#include "elix_graphics.h"

static elix_fpscounter fps;
static elix_program_info program_info;

int main(int UNUSEDARG argc, char UNUSEDARG * argv[])
{
	//elix_os_clipboard_put("asdf hello");
	elix_os_window * w = elix_os_window_create({{600, 400}}, {4,4}, "Test Example");

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );

	fps.start();

	rbgabuffer_BeginPath(bitmap_context);
	rbgabuffer_Rect(bitmap_context, 80, 80, 120,30);
	rbgabuffer_FillColor(bitmap_context, 0xFFFF00FF);
	rbgabuffer_Fill(bitmap_context);

	rbgabuffer_BeginPath(bitmap_context);
	rbgabuffer_MoveTo(bitmap_context, 20.0, 0.0);
	rbgabuffer_LineTo(bitmap_context, 40.0, 50.0);
	rbgabuffer_LineTo(bitmap_context, 20.0, 40.0);
	rbgabuffer_LineTo(bitmap_context, 0.0, 50.0);
	rbgabuffer_ClosePath(bitmap_context);
	rbgabuffer_FillColor(bitmap_context, 0xFFFFf000);
	rbgabuffer_Fill(bitmap_context);

	rbgabuffer_FillText(bitmap_context, "Test 🐨 🐱‍🚀 sadf", 10, 16, 500);

	while(elix_os_window_handle_events(w) ) {
		if ( w->flags & EOE_WIN_CLOSE ) {
			elix_os_window_destroy(w);
			break;
		}
		//update_buffer_randomly(bitmap_context->memory);

		fps.update();
		elix_os_window_render(w);
		//elix_os_system_idle(16000);
	}
	
	elix_os_window_destroy( w );
	delete w;

	return 0;
}



