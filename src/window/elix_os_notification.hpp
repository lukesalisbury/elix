#include "elix_core.h"

#if defined PLATFORM_WINDOWS
	#include "elix_os_notification_windows.hpp"
#elif defined PLATFORM_LINUX
	#include "elix_os_notification_wayland.hpp"
#else
	#error "Unsupported platform"
#endif

elix_window_notification elix_window_notification_new( const char * application );
void elix_window_notification_watch( elix_window_notification & note);

elix_window_notification_message * elix_window_notification_add( elix_window_notification & handler, elix_window_notification_settings & setting, void (*callback) (elix_window_notification_message * data), void * data );
void elix_window_notification_message_delete( /*elix_window_notification_message*/ void  * message);

void elix_window_notification_tick( elix_window_notification & note );
void elix_window_notification_close( elix_window_notification & note);

elix_window_notification_settings elix_window_notification_settings_create( const char * subject, const char * action_name, const char* action_text, const  char* body, ... );
