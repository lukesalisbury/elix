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

#ifndef ELIX_CONSENT_HPP
#define ELIX_CONSENT_HPP

#include "elix_core.h"

enum ELIX_CONSENT_LEVEL {
	ECONSENT_NONE = 0,
	ECONSENT_READ_OWNER, // First Person
	ECONSENT_READ_SAME_DOMAIN, // First Party
	ECONSENT_READ_VIA_DOMAIN, // Second Party
	ECONSENT_READ, // Third Party
	ECONSENT_WRITE_OWNER, // First Person
	ECONSENT_WRITE_SAME_DOMAIN, // First Party
	ECONSENT_WRITE_VIA_DOMAIN, // Second Party
	ECONSENT_WRITE, // Third Party
	ECONSENT_ADMIN= 0xEFFFF,
	ECONSENT_ANY = 0xffff
};


struct elix_consent_file {
	uint64_t user_id;
	uint32_t allowed; //See ELIX_CONSENT_LEVEL
	char * prefix;
};

struct elix_consent {
	uint16_t count;
	uint16_t type;
	union {
		elix_consent_file * files;
	} array;
};

#endif // ELIX_CONSENT_HPP
