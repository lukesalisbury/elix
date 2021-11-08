
#include "elix_endian.hpp"
#include "elix_html.hpp"
#include "elix_fpscounter.hpp"
#include "elix_os_window.hpp"

#include "elix_rgbabuffer.hpp"

#include "elix_cstring.hpp"
#include "elix_program.hpp"
#include "elix_os.hpp"

#define EXPORT extern "C" __declspec( dllexport )

uint32_t rendertreeitem_to_rgbabuffer(elix_rendertree_item * item, rbgabuffer_context * ctx) {

	switch (item->data_type) {
		case ERTD_STRING:
			
			if ( item->data ) {
				std::string * str = static_cast<std::string *>(item->data);
				rbgabuffer_FillColor(ctx, item->render_style.colour.hex);
				rbgabuffer_FillText(ctx, "Testing", item->render_style.x, item->render_style.y, item->render_style.width);
			}
			
			break;
/*
		case ERTD_EMPTY:
			break;
		case ERTD_IMAGE:
			break;
		case ERTD_EXTERNAL:
			break;
		case ERTD_COUNT:
		case ERTD_UPDATE:
		break;
*/
		default:
			rbgabuffer_BeginPath(ctx);
			rbgabuffer_MoveTo(ctx, item->render_style.x, item->render_style.y);
			rbgabuffer_LineTo(ctx, item->render_style.x + item->render_style.width, item->render_style.y);
			rbgabuffer_LineTo(ctx, item->render_style.x + item->render_style.width, item->render_style.y + item->render_style.height);
			rbgabuffer_LineTo(ctx, item->render_style.x, item->render_style.y + item->render_style.height);
			rbgabuffer_FillColor(ctx, item->render_style.backgroundColour.hex);
			rbgabuffer_Fill(ctx);
			break;
	}

	
	for(elix_rendertree_item * child : item->children) {
		rendertreeitem_to_rgbabuffer(child, ctx);
	}

	return 0;
}
uint32_t rendertree_to_rgbabuffer(elix_rendertree * tree, rbgabuffer_context * ctx, uint8_t redraw_all) {
	ASSERT(tree);
	ASSERT(ctx);
	if ( tree->root ) {
		rendertreeitem_to_rgbabuffer(tree->root, ctx);
	} else {
		LOG_ERROR("No Render Tree to render");
	}

	return 0;
}


EXPORT void Loop( elix::html::document & html, rbgabuffer_context * bitmap_context ) {
	
	elix_rendertree tree = elix::html::build_render_tree(&html, bitmap_context->dimensions);
	rendertree_to_rgbabuffer(&tree, bitmap_context, 1);
}