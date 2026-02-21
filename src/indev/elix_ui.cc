#include "elix_ui.h"

/*
elix_rendertree_computedstyle g_elix_ui_default_style[ELIX_UI_COUNT][ELIX_UI_STATE_COUNT] = {
	// ELIX_UI_WINDOW
	{
		{0, 0, 320, 320, {0x0FF000FF}, {0xFF0F00FF}, ERT_BLOCK}, // Normal
		{0, 0, 320, 320, {0x0FF000FF}, {0xFF0F00FF}, ERT_BLOCK}, // Hover
		{0, 0, 320, 320, {0x0FF000FF}, {0xFF0F00FF}, ERT_BLOCK}, // Active
		{0, 0, 320, 320, {0x0FF000FF}, {0xFF0F00FF}, ERT_BLOCK}  // Disabled
	},
	// ELIX_UI_DIALOG
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_THROBBER
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_CHECKBOX
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_TEXT
	{
		{{0xFF0000FF}, {0xFFFFFFFF}},
		{{0xFF0000FF}, {0xFFFFFFFF}},
		{{0xFF0000FF}, {0xFFFFFFFF}},
		{{0xFF0000FF}, {0xFFFFFFFF}}
	},
	// ELIX_UI_INPUTTEXT
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_INPUTSECRET
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_BUTTON
	{
		{{0xFFFFFFFF}, {0x000000FF}BLOCK},
		{{0xFFFFFFFF}, {0x000000FF}BLOCK},
		{{0xFFFFFFFF}, {0x000000FF}BLOCK},
		{{0xFFFFFFFF}, {0x000000FF}BLOCK}
	},
	// ELIX_UI_LIST
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_IMAGE
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_BOX
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_GRID
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	},
	// ELIX_UI_EXTERNAL
	{
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}},
		{{0xFFFFFFFF}, {0x000000FF}}
	}
};
*/

elix_ui * elix_ui_create( uint32_t width, uint32_t height ) {
	elix_ui * ui = ALLOCATE(elix_ui, 1);
	ui->dimensions.width = width;
	ui->dimensions.height = height;

	ui->layout_default[ELIX_UI_WINDOW] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_DIALOG] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_WINDOW] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_THROBBER] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_CHECKBOX] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_TEXT] = {0, 0, 320, 320, ERT_INLINE};
	ui->layout_default[ELIX_UI_INPUTTEXT] = {0, 0, 320, 320, ERT_INLINEBLOCK};
	ui->layout_default[ELIX_UI_INPUTSECRET] = {0, 0, 320, 320, ERT_INLINEBLOCK};
	ui->layout_default[ELIX_UI_BUTTON] = {0, 0, 320, 320, ERT_INLINEBLOCK};
	ui->layout_default[ELIX_UI_LIST] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_IMAGE] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_BOX] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_GRID] = {0, 0, 320, 320, ERT_BLOCK};
	ui->layout_default[ELIX_UI_EXTERNAL] = {0, 0, 320, 320, ERT_BLOCK};

	ui->style_default[ELIX_UI_WINDOW][ELIX_UI_STATE_NORMAL] 	= {{0x0FF000FF}, {0xFF0F00FF}}; // Normal
	ui->style_default[ELIX_UI_WINDOW][ELIX_UI_STATE_HOVER] 		= {{0x0FF000FF}, {0xFF0F00FF}}; // Hover
	ui->style_default[ELIX_UI_WINDOW][ELIX_UI_STATE_ACTIVE] 	= {{0x0FF000FF}, {0xFF0F00FF}}; // Active
	ui->style_default[ELIX_UI_WINDOW][ELIX_UI_STATE_DISABLED] 	= {{0x0FF000FF}, {0xFF0F00FF}}; // Disabled

	ui->style_default[ELIX_UI_DIALOG][ELIX_UI_STATE_NORMAL] 	= {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_DIALOG][ELIX_UI_STATE_HOVER] 		= {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_DIALOG][ELIX_UI_STATE_ACTIVE] 	= {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_DIALOG][ELIX_UI_STATE_DISABLED] 	= {{0xFFFFFFFF}, {0x000000FF}}; // Disabled
	
	ui->style_default[ELIX_UI_THROBBER][ELIX_UI_STATE_NORMAL] 	= {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_THROBBER][ELIX_UI_STATE_HOVER] 	= {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_THROBBER][ELIX_UI_STATE_ACTIVE] 	= {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_THROBBER][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_CHECKBOX][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_CHECKBOX][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_CHECKBOX][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_CHECKBOX][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_TEXT][ELIX_UI_STATE_NORMAL] = {{0xFF0000FF}, {0xFFFFFFFF}}; // Normal
	ui->style_default[ELIX_UI_TEXT][ELIX_UI_STATE_HOVER] = {{0xFF0000FF}, {0xFFFFFFFF}}; // Hover
	ui->style_default[ELIX_UI_TEXT][ELIX_UI_STATE_ACTIVE] = {{0xFF0000FF}, {0xFFFFFFFF}}; // Active
	ui->style_default[ELIX_UI_TEXT][ELIX_UI_STATE_DISABLED] = {{0xFF0000FF}, {0xFFFFFFFF}}; // Disabled

	ui->style_default[ELIX_UI_INPUTTEXT][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_INPUTTEXT][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_INPUTTEXT][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_INPUTTEXT][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_INPUTSECRET][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_INPUTSECRET][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_INPUTSECRET][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_INPUTSECRET][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_BUTTON][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_BUTTON][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_BUTTON][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_BUTTON][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_LIST][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_LIST][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_LIST][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_LIST][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_IMAGE][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_IMAGE][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_IMAGE][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_IMAGE][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_BOX][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_BOX][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_BOX][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_BOX][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_GRID][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_GRID][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_GRID][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_GRID][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled

	ui->style_default[ELIX_UI_EXTERNAL][ELIX_UI_STATE_NORMAL] = {{0xFFFFFFFF}, {0x000000FF}}; // Normal
	ui->style_default[ELIX_UI_EXTERNAL][ELIX_UI_STATE_HOVER] = {{0xFFFFFFFF}, {0x000000FF}}; // Hover
	ui->style_default[ELIX_UI_EXTERNAL][ELIX_UI_STATE_ACTIVE] = {{0xFFFFFFFF}, {0x000000FF}}; // Active
	ui->style_default[ELIX_UI_EXTERNAL][ELIX_UI_STATE_DISABLED] = {{0xFFFFFFFF}, {0x000000FF}}; // Disabled



	ui->widgets[0].parent = nullptr;
	ui->widgets[0].type = ELIX_UI_WINDOW;
	ui->widgets[0].position = {0, 0};
	ui->widgets[0].dimensions = {width, height};



	ui->widgets[0].ui = ui;
	ui->root = &ui->widgets[0];
	return ui;
}

uint32_t elix_ui_import_text(elix_ui * ui, const char * text, size_t text_length) {
	ASSERT(ui);
	ASSERT(text);

	uint8_t text_index = 0;
	uint32_t text_ref = 0;
	for (size_t index = 0; index < text_length; index++) {
		ui->text_buffer[index] = text[index];
		if ( text[index] == '\0'  ) {
			ui->text[text_index++] = &ui->text_buffer[text_ref];
			text_ref = index+1;
		}
	}


	return 1;
}


uint32_t elix_widget_add_collision( elix_ui * ui, elix_widget * reference, elix_sv32_2 offset, elix_uv32_2 dimensions) {
	ASSERT(reference);
	uint16_t next = 0;
	while ( next < 512 && ui->collision[next].reference != nullptr ) {
		next++;
	}
	if ( next < 512 ) {
		ui->collision[next].reference = reference;
		ui->collision[next].position = offset;
		ui->collision[next].dimensions = dimensions;
		return 1;
	}
	LOG_ERROR("No space for collision");
	return 0;
}



uint32_t elix_widget_add_child( elix_widget * parent, uint8_t type, elix_sv32_2 position, elix_uv32_2 dimensions, uint32_t resource ) {
	elix_widget * child = nullptr;
	uint16_t next = 0;
	while ( next < 256 && parent->ui->widgets[next].type != ELIX_UI_EMPTY ) {
		next++;
	}
	if ( next < 256 ) {
		parent->ui->widgets[next].parent = parent;
		parent->ui->widgets[next].position = position;
		parent->ui->widgets[next].dimensions = dimensions;
		parent->ui->widgets[next].type = type;
		parent->ui->widgets[next].resource = resource;
		parent->ui->widgets[next].ui = parent->ui;


		switch (type) {
			case ELIX_UI_WINDOW:
				/* code */
				break;
			
			default:
				//elix_widget_add_collision(parent->ui, &parent->ui->widgets[next]);
				break;
		}



		return 1;
	}
	LOG_ERROR("No space for child widget");
	return 0;
}
