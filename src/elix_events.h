/***********************************************************************************************************************
 Copyright (c) Luke Salisbury
 This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
 liable for any damages arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
 it and redistribute it freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
	If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is
	not required.
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
	software.
 3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/

#ifndef ELIX_EVENTS_HPP
#define ELIX_EVENTS_HPP

#include "elix_core.h"

enum elix_os_event_type {
	EOE_NONE = 0,
	EOE_EXIT = 1 << 0, // Force quit
	EOE_QUIT = 1 << 1, // Requesting quit

	EOE_FILE_CHANGE = 1 << 5,
	EOE_CLIPBOARD_CHANGE = 1 << 6,
	EOE_USER_CHANGE = 1 << 7,

	EOE_INPUT = 1 << 10,
	EOE_INPUT_KEYUP = 1 << 11,
	EOE_INPUT_KEYDOWN = 1 << 12,
	EOE_INPUT_POINTER = 1 << 13,
	EOE_INPUT_AXIS = 1 << 13,

	EOE_WIN_CLOSE = 1 << 20,
	EOE_WIN_RESIZE = 1 << 21,
	EOE_WIN_MOVE = 1 << 22,
	EOE_WIN_FULLSCREEN = 1 << 23,
	EOE_WIN_MAXIMISE = 1 << 24,
	EOE_WIN_MINIMISE = 1 << 25,
	EOE_WIN_ACTIVE  = 1 << 26,
	EOE_WIN_REFRESH = 1 << 27,
	EOE_WIN_INACTIVE  = 1 << 28,

	EOE_INVERT = 1 << 30,
	EOE_USEREVENT = 1 << 31,
	EOE_ALL = 0xFFFFFFFF
};

typedef struct elix_device_value { uint32_t number; int32_t value; } elix_device_value;


typedef struct elix_os_event {
	uint32_t type;
	uint32_t timestamp;
	union {
		//uint64_t raw;
		elix_device_value device;
		elix_sv32_2 position;
	};

} elix_os_event;

#define ELIX_EVENT_QUEUE_SIZE 64

typedef struct elix_os_event_queue {
	uint32_t filter; //Default: EOE_ALL
	elix_os_event event[ELIX_EVENT_QUEUE_SIZE];
	uint8_t push_index;
	uint8_t pull_index;
} elix_os_event_queue;

//void elix_os_event_set_filter(uint32_t filter = EOE_ALL);


inline void elix_os_event_push_eventtype( elix_os_event_queue * queue, uint32_t type ) {
	if ( queue->filter & type ) {
		if ( queue->push_index < ELIX_EVENT_QUEUE_SIZE ) {
			queue->event[ queue->push_index ].type = type;
			queue->push_index++;
		}
	}
}

inline void elix_os_event_push_event( elix_os_event_queue * queue, elix_os_event event ) {
	if ( queue->filter & event.type ) {
		if ( queue->push_index < ELIX_EVENT_QUEUE_SIZE ) {
			queue->event[ queue->push_index ] = event;
			queue->push_index++;
		}
	}
}
inline void elix_os_event_end_frame( elix_os_event_queue * queue, elix_os_event event ) {
	queue->push_index = 0;

}


#endif // ELIX_EVENTS_HPP
