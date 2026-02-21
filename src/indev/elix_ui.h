#ifndef ELIX_UI_HEADER
#define ELIX_UI_HEADER


#include "elix/elix_core.h"
#include "elix_rendertree.h"

#define BUTTON_NOTHING						0
#define BUTTON_RELEASED						1
#define BUTTON_PRESSED						2
#define BUTTON_HELD							3

enum {
	ELIX_UI_EMPTY,
	ELIX_UI_WINDOW,
	ELIX_UI_DIALOG,
	ELIX_UI_THROBBER,
	ELIX_UI_CHECKBOX,
	ELIX_UI_TEXT,
	ELIX_UI_INPUTTEXT,
	ELIX_UI_INPUTSECRET,
	ELIX_UI_BUTTON,
	ELIX_UI_LIST,
	ELIX_UI_IMAGE,
	ELIX_UI_BOX,
	ELIX_UI_GRID,
	ELIX_UI_EXTERNAL,
	ELIX_UI_COUNT
} WidgetTypes;

enum {
	ELIX_UI_STATE_NORMAL,
	ELIX_UI_STATE_HOVER,
	ELIX_UI_STATE_ACTIVE,
	ELIX_UI_STATE_DISABLED,
	ELIX_UI_STATE_COUNT
} elix_widget_state;

enum {
	ELIX_UI_EVENT_CLICK,
	ELIX_UI_EVENT_HOVER,
	ELIX_UI_EVENT_DRAG,
	ELIX_UI_EVENT_RELEASE,
	ELIX_UI_EVENT_DROP,
	ELIX_UI_EVENT_SCROLL,
	ELIX_UI_EVENT_COUNT
} elix_widget_event_type;

typedef enum InputDevice {
	NOINPUT,
	KEYBOARD,
	MOUSEAXIS,
	MOUSEBUTTON,
	MOUSEWHEEL,
	CONTROLAXIS,
	CONTROLBUTTON,
	CONTROLHAT,
	CONTROLBALL,
	TOUCHSCREEN,
	VIRTUALBUTTON,
	VIRTUALAXIS,
	HARDWARE
} InputDevice;


struct elix_ui;

typedef struct elix_ui_event {
	uint8_t type;
	uint8_t state;
	uint32_t resource;
	elix_sv32_2 position;
	elix_uv32_2 dimensions;
} elix_ui_event;

typedef struct elix_widget {
	uint8_t type;
	uint32_t resource;
	uint32_t state;
	elix_sv32_2 position;
	elix_uv32_2 dimensions;

	elix_widget * parent;
	elix_widget * children;

	elix_rendertree2_item * render_item;
	elix_ui_event event[8];
	elix_ui * ui;
} elix_widget;

typedef struct elix_widget_collision {
	elix_widget * reference;
	elix_sv32_2 position;
	elix_uv32_2 dimensions;

} elix_widget_collision;

typedef uint32_t elix_ui_image_id;

typedef struct elix_ui_input {
	uint32_t mouse_button;
	int16_t mouse_pointer[2]; // x/y
	int32_t key_count;
	const bool * key_state;
} elix_ui_input;


typedef union elix_controller_value {
	int16_t raw;
	struct {
		uint16_t state:2;
		uint16_t timer:14;
	};
} elix_controller_value;

typedef struct elix_controller_button {
	InputDevice device;
	uint32_t device_number;
	int16_t sym;
	elix_controller_value value;
	elix_ui_image_id sprite;
} elix_controller_button;

typedef struct elix_controller_pointer {
	InputDevice device;
	uint32_t device_number;
	int16_t sym;
	int16_t value[2]; // x/y
	elix_ui_image_id sprite[2];
} elix_controller_pointer;

typedef struct elix_controller_axis {
	InputDevice device;
	uint32_t device_number;
	int16_t sym[6]; // x-x+/y-y+/z-z++
	int16_t value[3]; // x/y/z
	elix_ui_image_id sprite[6];  // x-x+/y-y+/z-z++
} elix_controller_axis;

struct elix_controller {
	char name[24];
	char texture[24];
	elix_controller_button button[24];
	elix_controller_axis left_stick;
	elix_controller_axis right_stick;
	elix_controller_pointer pointer;
};

typedef struct elix_ui {
	elix_uv32_2 dimensions;

	// Style references
	elix_rendertree2_computed_dimensions layout_default[ELIX_UI_COUNT];
	elix_rendertree2_computed_style style_default[ELIX_UI_COUNT][ELIX_UI_STATE_COUNT];
	
	// Text references
	char text_buffer[1024];
	char * text[256];

	// Image references

	// Collision
	elix_widget_collision collision[512];

	//
	elix_ui_input input_global;
	elix_controller * control_default;
	elix_controller controllers[5]; // 0 = default, 1-4 = gamepads

	// Widgets
	elix_widget * root, * active_widget;
	elix_widget widgets[256];
} elix_ui;

elix_ui * elix_ui_create( uint32_t width, uint32_t height );
uint32_t elix_ui_import_text(elix_ui * ui, const char * text, size_t text_length);


uint32_t elix_widget_add_child( elix_widget * parent, uint8_t type, elix_sv32_2 position, elix_uv32_2 dimensions, uint32_t resource );
uint32_t elix_widget_add_collision( elix_ui * ui, elix_widget * reference, elix_sv32_2 offset, elix_uv32_2 dimensions);


#endif //ELIX_UI_HEADER
