#include "elix_core.h"
#include "elix_html.h"
#include "elix_rendertree.h"

void elix_rendertree_itemlist_push(elix_rendertree_itemlist * nl, elix_rendertree_item * item);
elix_html_node elix_html_nodelist_get(elix_html_nodelist * nl, uint16_t index);


uint8_t elix_html_build_rendertree_item(elix_rendertree * tree, elix_html_node obj, elix_rendertree_item * render_item) {
	ASSERT(obj);


	//ELEMENT_FOREIGN,
	//ELEMENT_VOID,
	//ELEMENT_TEMPLATE,
	//ELEMENT_RAWTEXT,
	//ELEMENT_RAWTEXTAREA,
	//ELEMENT_NORMAL

	switch (obj->type) {
		case ELEMENT_RAWTEXT:
			render_item->render_style.backgroundColour.hex = 0x000000FF;
			render_item->data = &obj->textContent;
			render_item->data_type = ERTD_STRING;
			render_item->render_style.display = ERT_INLINEBLOCK;
			//printf("Text: '" pEHSP "'\n", EHSPp( ((elix_string_pointer*)(render_item->data))));

			break;
		case ELEMENT_NORMAL:
			render_item->data_type = ERTD_EMPTY;
			render_item->render_style.display = ERT_BLOCK;
			if ( render_item->parent ) {
				render_item->render_style.backgroundColour.hex = 0xFFFFFFFF;
				render_item->render_style.width = render_item->parent->render_style.width;
			} else  {
				render_item->render_style.backgroundColour.hex = 0xFF0000FF;
				render_item->render_style.width = tree->width;
				render_item->render_style.height = tree->height;
			}
			break;
		default:
			render_item->data_type = ERTD_EMPTY;
			break;
	}

	if ( obj->children.active.used ) {
		for (uint16_t index = 0; index < obj->children.active.used; index++) {
			elix_html_node q = elix_html_nodelist_get(&obj->children, index);
			if ( q ) {
				elix_rendertree_item * child_item = ALLOCATE(elix_rendertree_item, 1);
				child_item->parent = render_item;
				elix_rendertree_itemlist_push(&render_item->children, child_item);
				elix_html_build_rendertree_item(tree, q, child_item);
			} else {
				printf("Invalid Node %d\n", index);
			}
		}
	}
}

elix_rendertree elix_html_build_rendertree(elix_html_document * doc, elix_uv32_2 dimension) {
	elix_rendertree tree = {};
	if (doc->root) {
		tree.width = dimension.width;
		tree.height = dimension.height;
		tree.root = ALLOCATE(elix_rendertree_item, 1);
		elix_html_build_rendertree_item(&tree, doc->root, tree.root);
	} else {
		printf("No Root Node found\n");
	}

	return tree;
}

