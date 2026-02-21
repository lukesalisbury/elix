#ifndef ELIX_CONTROLLER_H
#define ELIX_CONTROLLER_H

#include "elix_ui.h"

extern elix_controller default_keyboard;
extern elix_controller default_gamepad;

int16_t elix_controller_get_value(elix_ui * ui, uint32_t & bits, size_t index, InputDevice device, uint32_t device_number, int32_t symbol);

function_results elix_controller_update(elix_ui * ui );


#endif // ELIX_CONTROLLER_H