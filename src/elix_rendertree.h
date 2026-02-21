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
#ifndef ELIX_RENDERTREE_HEADER
#define ELIX_RENDERTREE_HEADER

#include "elix_core.h"


typedef int32_t elix_dimension;

typedef enum {
	ERTD_EMPTY = 0x00,
	ERTD_STRING = 0x01,
	ERTD_IMAGE = 0x02,
	ERTD_EXTERNAL = 0x04,
	ERTD_COUNT = 0x08,
	ERTD_UPDATE = 0x80
} elix_rendertree_datatype;

typedef enum {
	ERT_INLINE,
	ERT_INLINEBLOCK,
	ERT_BLOCK
} elix_rendertree_display;

typedef enum {
	ERTS_NORMAL = 0x00,
	ERTS_CLIPPED = 0x01,
	ERTS_HIDDEN = 0x02
} elix_rendertree_state;


typedef struct elix_rendertree_itemlist elix_rendertree_itemlist;
typedef struct elix_rendertree_item elix_rendertree_item;
typedef struct elix_rendertree_itempool {
	uint8_t used;
	data_pointer values[8];
} elix_rendertree_itempool;

typedef struct elix_rendertree_itemlist {
	elix_rendertree_itempool active;
	elix_rendertree_itemlist * next;
} elix_rendertree_itemlist;


typedef struct elix_rendertree_styletext {
	char * text;
	uint8_t animated, changed, unknown1, unknown2;
} elix_rendertree_styletext;


typedef struct elix_rendertree_style {
	elix_rendertree_styletext top, left, width, height;
} elix_rendertree_style;

typedef struct elix_rendertree_computedstyle {
	elix_dimension x, y, width, height;
	elix_colour backgroundColour;
	elix_colour colour;
	elix_rendertree_display display;
} elix_rendertree_computedstyle;

typedef struct elix_rendertree_item {
	elix_rendertree_computedstyle render_style;
	elix_rendertree_style style;
	data_pointer data;
	elix_rendertree_datatype data_type;
	elix_rendertree_itemlist children;
	elix_rendertree_item * parent;
} elix_rendertree_item;

typedef struct elix_rendertree {
	elix_rendertree_item * root;
	elix_dimension width, height;
} elix_rendertree;



#endif // ELIX_RENDERTREE_HEADER
