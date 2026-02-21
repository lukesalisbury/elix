/***********************************************************************************************************************
Copyright © Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If
   you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not
   required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
   software.
3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/
#include "elix_rendertree.h"
#include "elix_rgbabuffer.h"

elix_rendertree_item * elix_rendertree_itemlist_next(elix_rendertree_itemlist * list, size_t count) {


}

uint32_t elix_rendertreeitem_to_rgbabuffer(elix_rendertree_item * item, rbgabuffer_context * ctx) {

	switch (item->data_type) {
		case ERTD_STRING:
			
			if ( item->data ) {
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

	
	elix_rendertree_item * child = nullptr;
	size_t loop_count = 0; ; 

	while ( (child = elix_rendertree_itemlist_next( &item->children, loop_count)) )
	{
		elix_rendertreeitem_to_rgbabuffer(child, ctx);

		loop_count++;
	}

	return 0;
}

uint32_t elix_rendertree_to_rgbabuffer(elix_rendertree * tree, rbgabuffer_context * ctx, uint8_t redraw_all) {
	ASSERT(tree);
	ASSERT(ctx);
	if ( tree->root ) {
		LOG_MESSAGE("Buffer Size " pZD "x" pZD, ctx->dimensions.width, ctx->dimensions.height);
		elix_rendertreeitem_to_rgbabuffer(tree->root, ctx);

	} else {
		LOG_ERROR("No Render Tree to render");
	}

	return 0;
}
