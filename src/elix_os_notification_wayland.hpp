#ifndef ELIX_OSNOTICATION_WAYLAND_HPP
#define ELIX_OSNOTICATION_WAYLAND_HPP

#include "elix_core.h"

#include <unistd.h>
#include <dbus-1.0/dbus/dbus.h>

struct elix_window_notification
{
	/* data */
	char * application;
	unsigned id;
	char * icon;
	char * summary;
	char * body;
	char * default_action;
	char * action_text;

	/* */
	uint32_t exiting;
	DBusConnection * connection;
	DBusMessage * message;

	void (*callback) (char * action_key);
};


#endif
