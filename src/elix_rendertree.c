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

void elix_rendertree_itemlist_push(elix_rendertree_itemlist * nl, elix_rendertree_item *item) {
	if ( !nl ) {
		LOG_ERROR("Invalid elix_rendertree_itemlist");
		return;
	}
	if ( nl->active.used < 8 ) {
		nl->active.values[nl->active.used & 0x07] = item;
		nl->active.used++;
		return;
	}
	if ( !nl->next ) {
		nl->next = ALLOCATE(elix_rendertree_itemlist, 1);
	}
	if ( !nl->next ) {
		LOG_MESSAGE("Can not create more elix_rendertree_itemlist");
		return;
	}
	elix_rendertree_itemlist_push(nl->next, item);
}

elix_rendertree_item * elix_rendertree_itemlist_get(elix_rendertree_itemlist * list, size_t index) {

	if (!list || !list->active.used) {
		LOG_ERROR("Invalid elix_html_nodelist");
		return nullptr;
	}
	if (list->active.used > index) {
		return list->active.values[(index) & 0x07];
	}
	if (list->next) {
		return elix_rendertree_itemlist_get(list->next, index - 8);
	}
	return nullptr;

}

//TODO: cache 
#include "elix_parse.h"
#include "elix_cstring.h"
void rgbabuffer__fillChar(rbgabuffer_context* ctx, rgbabuffer_font * font, uint32_t character, float * x, float *y, uint32_t next_character);
void rbgabuffer_FillString(rbgabuffer_context* ctx, elix_string_pointer * text, float x, float y, float maxWidth) {

	char * object = (char*)text->string;
	char * next_object = object;
	uint32_t current_character = 0, next_character = 0;
	while ( (current_character = elix_cstring_next_character(object, &next_object)) > 0  ) {
		next_character = elix_cstring_peek_character(next_object);
		rgbabuffer__fillChar(ctx, ctx->loaded_font, current_character, &x, &y, next_character);
		object = next_object;
	}
}

uint32_t elix_rendertreeitem_to_rgbabuffer(elix_rendertree_item * item, rbgabuffer_context * ctx) {

	switch (item->data_type) {
		case ERTD_STRING:
			if ( item->data ) {
				//data is a elix_string_pointer

				rbgabuffer_FillColor(ctx, item->render_style.colour.hex);
				rbgabuffer_FillString(ctx, (elix_string_pointer*)item->data, item->render_style.x, item->render_style.y, item->render_style.width);
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


	if ( item->children.active.used ) {
		for (uint16_t index = 0; index < item->children.active.used; index++) {
			elix_rendertree_item * q = elix_rendertree_itemlist_get(&item->children, index);

			if ( q ) {
				elix_rendertreeitem_to_rgbabuffer(q, ctx);
			} else {
				printf("Invalid Node %d\n", index);
			}
		}
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
