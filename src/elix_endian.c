/***********************************************************************************************************************
Copyright © Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If
   you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not
   required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
   software.
3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/

#include "elix_core.h"

#if defined PLATFORM_SDL2_ONLY
	#define ENDIAN_SDL 1
#elif defined PLATFORM_NDS
	//#include <sys/socket.h>
	//#define ENDIAN_NETWORK 1
#elif defined PLATFORM_DREAMCAST
	//#include <endian.h>
	//#define ENDIAN_NETWORK 1
#elif defined PLATFORM_GAMECUBE || defined PLATFORM_WII
	//#include <network.h>
	//#define ENDIAN_NETWORK 1
#endif


#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	// Do Nothing, big endian is network endian
	uint32_t elix_endian_network32(uint32_t number) {
		return number;
	}
	uint16_t elix_endian_network16(uint16_t number) {
		return number;
	}
	uint32_t elix_endian_host32(uint32_t number) {
		return number;
	}
	uint16_t elix_endian_host16(uint16_t number) 	{
		return number;
	}
#elif defined ENDIAN_NETWORK
	uint32_t elix_endian_network32(uint32_t number) {
		return htonl(number);
	}
	uint16_t elix_endian_network16(uint16_t number) {
		return htons(number);
	}
	uint32_t elix_endian_host32(uint32_t number) {
		return ntohl(number);
	}
	uint16_t elix_endian_host16(uint16_t number) {
		return ntohs(number);
	}
#elif defined ENDIAN_SDL
	uint32_t elix_endian_network32(uint32_t number) {
		return SDL_SwapBE32(number);
	}
	uint16_t elix_endian_network16(uint16_t number) {
		return SDL_SwapBE16(number);
	}
	uint32_t elix_endian_host32(uint32_t number) {
		return SDL_Swap32(number);
	}
	uint16_t elix_endian_host16(uint16_t number) {
		return SDL_Swap16(number);
	}
#elif defined _MSC_VER
	// TODO: UNTESTED
	uint32_t elix_endian_network32(uint32_t number) {
		return _byteswap_ulong(number);
	}
	uint16_t elix_endian_network16(uint16_t number) {
		return _byteswap_ushort(number);
	}
	uint32_t elix_endian_host(uint32_t number) {
		return _byteswap_ulong(number);
	}
	uint16_t elix_endian_host(uint16_t number) {
		return _byteswap_ushort(number);
	}
#else
	uint32_t elix_endian_network32(uint32_t number) {
		return __builtin_bswap32(number);
	}
	uint16_t elix_endian_network16(uint16_t number) {
		return __builtin_bswap16(number);
	}
	uint32_t elix_endian_host32(uint32_t number) {
		return __builtin_bswap32(number);
	}
	uint16_t elix_endian_host16(uint16_t number) {
		return __builtin_bswap16(number);
	}

#endif

