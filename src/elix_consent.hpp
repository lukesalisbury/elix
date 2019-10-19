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
