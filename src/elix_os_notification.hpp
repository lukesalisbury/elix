#include "elix_core.h"


struct elix_window_notification_action
{
void (*callback) (char * action_key);
};

#if defined PLATFORM_WINDOWS
	#include "elix_os_notification_windows.hpp"
#elif defined PLATFORM_LINUX
	#include "elix_os_notification_wayland.hpp"
#else
	#error "Unsupported platform"
#endif



elix_window_notification elix_window_notification_new( char * application, unsigned id, char * icon, char *summary, char * body, char * default_action,	char * action_text, elix_window_notification_action callback );
void elix_window_notification_watch( elix_window_notification & note);
void elix_window_notification_close( elix_window_notification & note);
