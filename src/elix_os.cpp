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

#include "elix_os.hpp"
#include "elix_cstring.hpp"
#include <string.h>



#if defined PLATFORM_WINDOWS
	#include "elix_os_directory_win.cpp"
	#include "elix_os_win.cpp"
#elif defined PLATFORM_LINUX
	#include "elix_os_directory_posix.cpp"
	#include "elix_os_linux.cpp"
#elif defined PLATFORM_3DS
	#include "elix_os_directory_posix.cpp"
#elif defined PLATFORM_NXSWITCH
	#include "elix_os_directory_posix.cpp"
#elif defined PLATFORM_SDL2
	#include "elix_os_directory_posix.cpp"
#else
	#error "Unsupported platform"
#endif
