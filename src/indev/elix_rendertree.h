#ifndef ELIX_RENTERTREE_HEADER
#define ELIX_RENTERTREE_HEADER

#include "../elix_core.h"

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

enum elix_rendertree_state {
	ERTS_NORMAL = 0x00,
	ERTS_CLIPPED = 0x01,
	ERTS_HIDDEN = 0x02
};

struct elix_rendertree2_styletext {
	char * text = nullptr;
	uint8_t animated = 0, changed = 0, unknown1, unknown2;
};

struct elix_rendertree2_style {
	elix_rendertree2_styletext top, left, width, height;
};

struct elix_rendertree2_resource {
	elix_rendertree_datatype data_type = ERTD_EMPTY;
	uint16_t index = 0;
	data_pointer data = nullptr;
};


struct elix_rendertree2_computed_style {
	elix_colour backgroundColour = {0xFFFFFFFF};
	elix_colour colour = {0x000000FF};
};

struct elix_rendertree2_computed_dimensions {
	elix_dimension x = 0, y = 0, width = 320, height = 320;
	elix_rendertree_display display = ERT_BLOCK;
};

struct elix_rendertree2_item {
	elix_rendertree2_computed_dimensions render_layout;
	elix_rendertree2_computed_style render_style;
	elix_dimension clip_x1 = 0, clip_y1 = 0, clip_x2 = 320, clip_y2 = 320;
	uint32_t render_state = ERTS_NORMAL;
	elix_rendertree2_style style;
	elix_rendertree2_resource data;
	elix_rendertree2_item * parent = nullptr;
};

struct elix_rendertree2 {
	elix_dimension width = 320, height = 320;
	elix_rendertree2_item items[256];
	uint16_t item_count = 0;
};

#endif //ELIX_RENTERTREE_HEADER