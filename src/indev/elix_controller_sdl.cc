#include "elix/elix_core.h"
#include "elix_ui.h"
#include <SDL3/SDL.h>

elix_controller default_keyboard = { 
	"Keyboard",
	"Unknown", 
	{
		{ KEYBOARD, 0, SDL_SCANCODE_A, 0, 0 }, // BUTTON_DOWN
		{ KEYBOARD, 0, SDL_SCANCODE_S, 0, 0 }, // BUTTON_LEFT
		{ KEYBOARD, 0, SDL_SCANCODE_D, 0, 0 }, // BUTTON_RIGHT
		{ KEYBOARD, 0, SDL_SCANCODE_Q, 0, 0 }, // BUTTON_UP
		{ KEYBOARD, 0, SDL_SCANCODE_W, 0, 0 }, // SHOULDER_LEFT
		{ KEYBOARD, 0, SDL_SCANCODE_E, 0, 0 }, // SHOULDER_RIGHT
		{ KEYBOARD, 0, SDL_SCANCODE_RETURN, 0, 0 }, // BUTTON_MENU
		{ KEYBOARD, 0, SDL_SCANCODE_RIGHT, 0, 0 }, // DPAD_RIGHT
		{ KEYBOARD, 0, SDL_SCANCODE_LEFT, 0, 0 }, // DPAD_LEFT
		{ KEYBOARD, 0, SDL_SCANCODE_DOWN, 0, 0 }, // DPAD_DOWN
		{ KEYBOARD, 0, SDL_SCANCODE_UP, 0, 0 }, // DPAD_UP
		{ MOUSEBUTTON, 0, 1, 0, 0 } // 
	},
	{	KEYBOARD, 0, {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEDOWN}, {0,0,0}	},
	{	MOUSEAXIS, 0, {0, 0, 1, 1, 2, 2}, {0,0,0} },
	{	MOUSEAXIS, 0, 0, {0, 0}, 0 }
};

elix_controller default_gamepad = { 
	"Standard Game pad",
	"Game pad",  
	{
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_SOUTH, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_EAST, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_WEST, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_NORTH, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_START, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_DPAD_UP, 0 },
		{ CONTROLBUTTON, 0, SDL_GAMEPAD_BUTTON_BACK, 0 }
	},
	{	CONTROLAXIS, 0, {-SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTX, -SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}, {0,0,0}	},
	{	CONTROLAXIS, 0, {-SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTX, -SDL_GAMEPAD_AXIS_RIGHTY, SDL_GAMEPAD_AXIS_RIGHTY, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}, {0,0,0} },
	{	CONTROLAXIS, 0, SDL_GAMEPAD_AXIS_RIGHTX, {0,0}, {0,0} }
};


int16_t elix_controller_get_value(elix_ui * ui, uint32_t & bits, size_t index, InputDevice device, uint32_t device_number, int32_t symbol) {
	if (device == NOINPUT) {
		return 0;
	}
	int16_t value = 0;

	SDL_Gamepad * controller = nullptr;



	switch ( device ) {
		case KEYBOARD: {
			if ( ui->input_global.key_state && symbol < ui->input_global.key_count && symbol >= 0 ) {
				value = ui->input_global.key_state[symbol];
			}
			break;
		}
		case MOUSEAXIS: {
			if ( symbol == 1 || symbol == 0 ) {
				value =  ui->input_global.mouse_pointer[symbol];
			}
			break;
		}
		case MOUSEBUTTON: {
			if ( symbol < 5 && symbol >= 0 ) {
				value = !!( ui->input_global.mouse_button & SDL_BUTTON_MASK(symbol));
			}
			break;
		}
		case CONTROLAXIS: {
			SDL_GamepadAxis s = (SDL_GamepadAxis)(symbol & 0x7FFFFFFF);
			if ( s > SDL_GAMEPAD_AXIS_INVALID && s < SDL_GAMEPAD_AXIS_COUNT ) {
				controller = SDL_GetGamepadFromID(device_number);
				int16_t r = (int16_t)SDL_GetGamepadAxis(controller, s) / 128;
				///TODO: Better Deadzone handling
				if ( s < SDL_GAMEPAD_AXIS_LEFT_TRIGGER ) {
					if ( 50 > r && r > -50) // Poor Deadzone
						r = 0;
				}
				value = r;
			}
			break;
		}
		case CONTROLBUTTON: {
			SDL_GamepadButton s = (SDL_GamepadButton)symbol;
			if ( s < SDL_GAMEPAD_BUTTON_COUNT ) {
				controller = SDL_GetGamepadFromID(device_number);
				value = (int16_t)SDL_GetGamepadButton(controller, s);
			}
			break;
		}
		case NOINPUT:
		default:
			return 0;
	}
	if ( value && index < 32) {
		bits |= 1 << index;
	}
	return value;
}

function_results elix_controller_update(elix_ui * ui ) {
	for (elix_controller & pad : ui->controllers) {
		uint32_t input_test = 0;

		for (uint8_t i = 0; i < 24; i++) {
			int16_t value = elix_controller_get_value(ui, input_test, i, pad.button[i].device, pad.button[i].device_number, pad.button[i].sym);
			if ( value ) {
				if ( pad.button[i].value.state == BUTTON_NOTHING ) {
					pad.button[i].value.state = BUTTON_PRESSED;
					pad.button[i].value.timer = 0;
				} else if ( pad.button[i].value.state == BUTTON_PRESSED ) {
					pad.button[i].value.state = BUTTON_HELD;
					//pad.button[i].value.timer += (uint16_t)app->timing.ms_delta;
				}
			} else if ( pad.button[i].value.state == BUTTON_HELD || pad.button[i].value.state == BUTTON_PRESSED) {
				pad.button[i].value.state = BUTTON_RELEASED; //Keep Timer value;
			} else {
				pad.button[i].value.state = BUTTON_NOTHING;
				pad.button[i].value.timer = 0;
			}
		}
		switch (pad.left_stick.device) {
			case KEYBOARD: {
				int16_t key1, key2;
				key1 = elix_controller_get_value(ui, input_test, 24, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[0]);
				key2 = elix_controller_get_value(ui, input_test, 24, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[1]);
				pad.left_stick.value[0] = ( key1 && !key2 ? -255 : ( !key1 && key2 ? 255 : 0) );

				key1 = elix_controller_get_value(ui, input_test, 25, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[2]);
				key2 = elix_controller_get_value(ui, input_test, 25, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[3]);
				pad.left_stick.value[1] = ( key1 && !key2 ? -255 : ( !key1 && key2 ? 255 : 0) );

				key1 = elix_controller_get_value(ui, input_test, 26, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[4]);
				key2 = elix_controller_get_value(ui, input_test, 26, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[5]);
				pad.left_stick.value[2] = ( key1 && !key2 ? -255 : ( !key1 && key2 ? 255 : 0) );
				break;
			}
			case CONTROLAXIS: {
				pad.left_stick.value[0] = elix_controller_get_value(ui, input_test, 24, CONTROLAXIS, pad.left_stick.device_number,pad.left_stick.sym[1]);
				pad.left_stick.value[1] = elix_controller_get_value(ui, input_test, 25, CONTROLAXIS, pad.left_stick.device_number,pad.left_stick.sym[3]);
				pad.left_stick.value[2] = elix_controller_get_value(ui, input_test, 26, CONTROLAXIS, pad.left_stick.device_number,pad.left_stick.sym[5]);
				break;
			}
			default:
				break;
		}
		switch (pad.right_stick.device) {
			case KEYBOARD: {
				int16_t key1, key2;
				key1 = elix_controller_get_value(ui, input_test, 27, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[0]);
				key2 = elix_controller_get_value(ui, input_test, 27, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[1]);
				pad.right_stick.value[0] = ( key1 && !key2 ? -255 : ( !key1 && key2 ? 255 : 0) );
	
				key1 = elix_controller_get_value(ui, input_test, 28, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[2]);
				key2 = elix_controller_get_value(ui, input_test, 28, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[3]);
				pad.right_stick.value[1] = ( key1 && !key2 ? -255 : ( !key1 && key2 ? 255 : 0) );

				key1 = elix_controller_get_value(ui, input_test, 29, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[4]);
				key2 = elix_controller_get_value(ui, input_test, 29, KEYBOARD, pad.left_stick.device_number,pad.left_stick.sym[5]);
				pad.right_stick.value[2] = ( key1 && !key2 ? -255 : ( !key1 && key2 ? 255 : 0) );
				break;
			}
			case CONTROLAXIS: {
				pad.right_stick.value[0] = elix_controller_get_value(ui, input_test, 27, CONTROLAXIS, pad.right_stick.device_number,pad.right_stick.sym[1]);
				pad.right_stick.value[1] = elix_controller_get_value(ui, input_test, 28, CONTROLAXIS, pad.right_stick.device_number,pad.right_stick.sym[3]);
				pad.right_stick.value[2] = elix_controller_get_value(ui, input_test, 29, CONTROLAXIS, pad.right_stick.device_number,pad.right_stick.sym[5]);
				break;
			}
			default:
				break;
		}

		switch (pad.pointer.device) {
			case MOUSEAXIS: {
				pad.pointer.value[0] = ui->input_global.mouse_pointer[0];
				pad.pointer.value[1] = ui->input_global.mouse_pointer[1];
				break;
			}
			case CONTROLAXIS: {
				///TODO: Update code to use relative movement 
				//pad.pointer.value[0] = elix_controller_get_value(ui, input_test, 30, CONTROLAXIS, pad.pointer.device_number, pad.pointer.sym);
				///NOTE: Don't just add 1 to symbol. In default setting, SDL_CONTROLLER_AXIS_RIGHTX + 1 = SDL_CONTROLLER_AXIS_RIGHTY
				//pad.pointer.value[1] = elix_controller_get_value(ui, input_test, 31, CONTROLAXIS, pad.pointer.device_number, pad.pointer.sym +1);
				break;
			}
			default:
				break;
		}

	}
	return RESULTS_SUCCESS;
}