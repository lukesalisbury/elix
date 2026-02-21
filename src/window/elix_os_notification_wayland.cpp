#include "elix_os_notification_wayland.hpp"
#include <thread>
#include "elix_cstring.h"

void elix_window_notification_message_delete( /*elix_window_notification_message*/ void  * message);

static DBusHandlerResult elix_window_notification_handler(DBusConnection * connection, DBusMessage * message, void * data) {
	const char *interface_name = dbus_message_get_interface(message);
	const char *member_name = dbus_message_get_member(message);

	elix_window_notification_message * notification = (elix_window_notification_message *)data;
	LOG_INFO("%s - %x", member_name, data);

	if (dbus_message_is_signal(message, "org.freedesktop.Notifications", "ActionInvoked")) {
		uint32_t q = 0;
		char * action_key = nullptr;
		
		dbus_message_get_args(message, nullptr, DBUS_TYPE_UINT32, &q, DBUS_TYPE_STRING, &action_key, DBUS_TYPE_INVALID);
		if( notification && notification->action_callback ) {
			notification->action_callback(notification);
		}
		dbus_message_unref(notification->message);

		return DBUS_HANDLER_RESULT_HANDLED;
	} else if (dbus_message_is_signal(message, "org.freedesktop.Notifications", "NotificationClosed")) 	{
		uint32_t q = 0, w = 0;
		dbus_message_get_args(message, nullptr, DBUS_TYPE_UINT32, &q, DBUS_TYPE_UINT32, &q, DBUS_TYPE_INVALID);
		elix_window_notification_message_delete(notification);
		dbus_message_unref(notification->message);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

elix_window_notification elix_window_notification_new( const char * application_name ){
	elix_window_notification note = {0};
	note.exiting = 0;
	note.application = "test";
	note.connection = dbus_bus_get(DBUS_BUS_SESSION, 0);

	DBusError error = {0};

	dbus_bus_add_match(note.connection, "type='signal',interface='org.freedesktop.Notifications',member='ActionInvoked'", &error);
	if (dbus_error_is_set(&error)) {
		LOG_MESSAGE("%s %s", error.name, error.message);
	}
	dbus_bus_add_match(note.connection, "type='signal',interface='org.freedesktop.Notifications',member='NotificationClosed'", &error);
	if (dbus_error_is_set(&error)) {
		LOG_MESSAGE("%s %s", error.name, error.message);
	}

	return note;
}

void elix_window_notification_message_delete( /*elix_window_notification_message*/ void  * message) {
	//message
}

elix_window_notification_message * elix_window_notification_add( elix_window_notification & handler, elix_window_notification_settings & settings, void (*callback) (elix_window_notification_message * ), void * data ) {
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
	
	elix_window_notification_message * note = new elix_window_notification_message();

	note->action_callback = callback;
	note->action_data = data;
	note->handler = &handler;
	note->message = dbus_message_new_method_call(
		"org.freedesktop.Notifications",
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications",
		"Notify");

	LOG_INFO("elix_window_notification_add - %x",  note);
	int timeout = 5 * 1000;
	DBusError error = {0};
	dbus_connection_add_filter(handler.connection, elix_window_notification_handler, note, &elix_window_notification_message_delete);
	if (dbus_error_is_set(&error)) {
		LOG_MESSAGE("%s %s", error.name, error.message);
	}

	DBusMessageIter args, array;

	#define ITER_APPEND(type, value) dbus_message_iter_append_basic(&args, type, value);

	dbus_message_iter_init_append(note->message, &args);
	ITER_APPEND(DBUS_TYPE_STRING, &handler.application);
	ITER_APPEND(DBUS_TYPE_UINT32, &settings.id);
	ITER_APPEND(DBUS_TYPE_STRING, &settings.icon);
	ITER_APPEND(DBUS_TYPE_STRING, &settings.summary);
	ITER_APPEND(DBUS_TYPE_STRING, &settings.body);

	// Actions
	if ( settings.default_action && settings.action_text ) {
		dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &array);
		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &settings.default_action);
		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &settings.action_text);
		dbus_message_iter_close_container(&args, &array);
	}
	// Hints
	dbus_message_iter_open_container(&args, 'a', "{sv}", &array);
	dbus_message_iter_close_container(&args, &array);

	ITER_APPEND(DBUS_TYPE_INT32, &timeout);

	#undef ITER_APPEND

	dbus_connection_send(handler.connection, note->message, 0);
	dbus_connection_flush(handler.connection);

	return note;

}



void elix_window_notification_remove( elix_window_notification & note, elix_window_notification_message & message) {
	dbus_message_unref(message.message);
}

void elix_window_notification_close( elix_window_notification & note) {
	note.exiting = 1;
	//dbus_message_unref(note.message);
	dbus_connection_unref(note.connection);
}

void elix_window_notification_tick( elix_window_notification & note ) {
	dbus_connection_read_write_dispatch(note.connection,1);
}

void elix_window_notification_watch( elix_window_notification & note ) {
	while ( !note.exiting ) {
		elix_window_notification_tick(note);
		std::this_thread::sleep_for(std::chrono::microseconds(33));
	}
}


elix_window_notification_settings elix_window_notification_settings_create( const char * subject, const char * action_name, const char* action_text, const char* body, ... ) {
	elix_window_notification_settings settings = {0};
	settings.icon = elix_cstring_new("dialog-information");
	if ( action_text && action_name ) {
		settings.action_text = elix_cstring_new(action_text);
		settings.default_action = elix_cstring_new(action_name);
	}

	settings.body = elix_cstring_new(body);
	settings.summary = elix_cstring_new(subject);

	return settings;
}


void elix_window_notification_settings_delete(  elix_window_notification_settings & settings ) {
	NULLIFY(settings.icon);
	NULLIFY(settings.action_text);
	NULLIFY(settings.default_action);
	NULLIFY(settings.body);
	NULLIFY(settings.summary);

}