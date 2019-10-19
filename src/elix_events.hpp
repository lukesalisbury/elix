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

	EOE_INVERT = 1 << 30,
	EOE_USEREVENT = 1 << 31,
	EOE_ALL = 0xFFFFFFFF
};

struct elix_device_value { uint32_t number; int32_t value; };


struct elix_os_event {
	uint32_t type;
	uint32_t timestamp;
	union {
		//uint64_t raw;
		elix_device_value device;
		elix_sv32_2 position;
	};

};

#define ELIX_EVENT_QUEUE_SIZE 64

struct elix_os_event_queue {
	uint32_t filter;
	elix_os_event queue[ELIX_EVENT_QUEUE_SIZE];
	uint8_t push_index = 0;
	uint8_t pull_index = 0;
};

void elix_os_event_set_filter(uint32_t filter = EOE_ALL);


inline void elix_os_event_push_event( elix_os_event_queue * queue, uint32_t type ) {
	if ( queue->filter & type ) {
		if ( queue->push_index < ELIX_EVENT_QUEUE_SIZE ) {

		}
	}


}


#endif // ELIX_EVENTS_HPP
