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
#ifndef ELIX_ENDIAN_H_
#define ELIX_ENDIAN_H_

#include "elix_core.h"

enum elix_endian_mode
{
	host,
	network
};

/**
   @brief Host to Network (32)
   @param number
   @return Big Endian value
 */
uint32_t elix_endian_network32( uint32_t number );
/**
   @brief Host to Network (16)
   @param number
   @return Big Endian value
*/
uint16_t elix_endian_network16( uint16_t number );
/**
   @brief Network to Host (32)
   @param number
   @return Host Endian value
*/
uint32_t elix_endian_host32( uint32_t number );
/**
   @brief Network to Host (16)
   @param number
   @return Host Endian value
*/
uint16_t elix_endian_host16( uint16_t number );

#endif

