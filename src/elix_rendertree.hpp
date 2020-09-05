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
#ifndef ELIX_RENDERTREE_HPP
#define ELIX_RENDERTREE_HPP

#include "elix_core.h"
#include <vector>

using elix_dimension = int32_t;

enum elix_rendertree_datatype {
	ERTD_EMPTY = 0x00,
	ERTD_STRING = 0x01,
	ERTD_IMAGE = 0x02,
	ERTD_EXTERNAL = 0x04,
	ERTD_COUNT = 0x08,
	ERTD_UPDATE = 0x80
};

enum elix_rendertree_display {
	ERT_INLINE,
	ERT_INLINEBLOCK,
	ERT_BLOCK
};

struct elix_rendertree_item;
using elix_rendertree_itemlist = std::vector<elix_rendertree_item*>;

struct elix_rendertree_styletext {
	char * text = nullptr;
	uint8_t animated = 0, changed = 0, unknown1, unknown2;
};

struct elix_rendertree_style {
	elix_rendertree_styletext top, left, width, height;
};

struct elix_rendertree_computedstyle {
	elix_dimension x = 0, y = 0, width = 320, height = 320;
	elix_colour backgroundColour = {0xFFFFFFFF};
	elix_colour colour = {0x000000FF};
	elix_rendertree_display display = ERT_BLOCK;
};

struct elix_rendertree_item {
	elix_rendertree_computedstyle render_style;
	elix_rendertree_style style;
	data_pointer data = nullptr;
	elix_rendertree_datatype data_type = ERTD_EMPTY;
	elix_rendertree_itemlist children;
	elix_rendertree_item * parent = nullptr;
};

struct elix_rendertree {
	elix_rendertree_item * root = nullptr;
	elix_dimension width = 320, height = 320;
};


#endif // ELIX_RENDERTREE_HPP
