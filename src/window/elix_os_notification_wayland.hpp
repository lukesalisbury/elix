#ifndef ELIX_OSNOTICATION_WAYLAND_HPP
#define ELIX_OSNOTICATION_WAYLAND_HPP

#include "elix_core.h"

#include <unistd.h>
#include <dbus-1.0/dbus/dbus.h>


typedef struct elix_window_notification_settings
{
	/* data */
	unsigned id;
	char * icon;
	char * summary;
	char * body;
	char * default_action;
	char * action_text;
	
} elix_window_notification_settings;




struct elix_window_notification
{
	/* data */
	const char * application;
	/* */
	uint32_t exiting;
	DBusConnection * connection;
	DBusMessage * messages[8];

};


struct elix_window_notification_message
{
	elix_window_notification * handler;
	DBusMessage * message;
	uint32_t index;

	void (*action_callback) (elix_window_notification_message * message);
	void * action_data;
};

#endif
