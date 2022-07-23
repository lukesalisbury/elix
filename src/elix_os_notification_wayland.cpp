#include "elix_os_notification_wayland.hpp"
#include <thread>

static DBusHandlerResult elix_window_notification_handler(DBusConnection * connection, DBusMessage * message, void * data) {
	const char *interface_name = dbus_message_get_interface(message);
	const char *member_name = dbus_message_get_member(message);

	elix_window_notification * notic = (elix_window_notification *)data;

	if (dbus_message_is_signal(message, "org.freedesktop.Notifications", "ActionInvoked")) {
		uint32_t q = 0;
		char * action_key = nullptr;
		
		dbus_message_get_args(message, nullptr, DBUS_TYPE_UINT32, &q, DBUS_TYPE_STRING, &action_key, DBUS_TYPE_INVALID);
		if( notic && notic->callback ) {
			notic->callback(action_key);
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	} else if (dbus_message_is_signal(message, "org.freedesktop.Notifications", "NotificationClosed")) 	{
		uint32_t q = 0, w = 0;
		dbus_message_get_args(message, nullptr, DBUS_TYPE_UINT32, &q, DBUS_TYPE_UINT32, &q, DBUS_TYPE_INVALID);
		return DBUS_HANDLER_RESULT_HANDLED;
	}


	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

elix_window_notification elix_window_notification_new(
	char * application,
	unsigned id,
	char * icon,
	char *summary,
	char * body,
	char * default_action,
	char * action_text
	//DBusHandlerResult (*notification_handler)(DBusConnection * connection, DBusMessage * message, void * data)

){
	elix_window_notification note;
	note.exiting = 0;
	note.connection = dbus_bus_get(DBUS_BUS_SESSION, 0);
	note.message = dbus_message_new_method_call(
		"org.freedesktop.Notifications",
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications",
		"Notify");

	DBusError error = {0};

	dbus_bus_add_match(note.connection, "type='signal',interface='org.freedesktop.Notifications',member='ActionInvoked'", &error);
	if (dbus_error_is_set(&error)) {
		LOG_MESSAGE("%s %s", error.name, error.message);
	}
	dbus_bus_add_match(note.connection, "type='signal',interface='org.freedesktop.Notifications',member='NotificationClosed'", &error);
	if (dbus_error_is_set(&error)) {
		LOG_MESSAGE("%s %s", error.name, error.message);
	}
	dbus_connection_add_filter(note.connection, elix_window_notification_handler, &note, nullptr);
	if (dbus_error_is_set(&error)) {
		LOG_MESSAGE("%s %s", error.name, error.message);
	}
	
/* - D-Feet testing
"test.app", 
0, 
"a3",
"b4", 
"c5", 
["action-name", "Action"], 
[], 
0
*/

	int timeout = 5 * 1000;

	DBusMessageIter args, array;

	#define ITER_APPEND(type, value) dbus_message_iter_append_basic(&args, type, value);

	dbus_message_iter_init_append(note.message, &args);
	ITER_APPEND(DBUS_TYPE_STRING, &application);
	ITER_APPEND(DBUS_TYPE_UINT32, &id);
	ITER_APPEND(DBUS_TYPE_STRING, &icon);
	ITER_APPEND(DBUS_TYPE_STRING, &summary);
	ITER_APPEND(DBUS_TYPE_STRING, &body);

	// Actions
	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &array);
	dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &default_action);
	dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &action_text);
	dbus_message_iter_close_container(&args, &array);

	// Hints
	dbus_message_iter_open_container(&args, 'a', "{sv}", &array);
	dbus_message_iter_close_container(&args, &array);

	ITER_APPEND(DBUS_TYPE_INT32, &timeout);

	#undef ITER_APPEND

	dbus_connection_send(note.connection, note.message, 0);
	dbus_connection_flush(note.connection);

	return note;
}

void elix_window_notification_close( elix_window_notification & note) {
	note.exiting = 1;
	dbus_message_unref(note.message);
	dbus_connection_unref(note.connection);
}

void elix_window_notification_watch( elix_window_notification & note) {
	while ( !note.exiting ) {
		dbus_connection_read_write_dispatch(note.connection,1);
		std::this_thread::sleep_for(std::chrono::microseconds(33));
	}
}