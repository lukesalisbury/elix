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

#ifndef ELIX_EXTRA_HPP
#define ELIX_EXTRA_HPP

#include "elix_core.h"
#include "elix_cstring.h"
#include "elix_os.h"
#include "elix_networksocket.h"

typedef struct elix_http_request {
	char * headers_buffer;
	char * body_buffer;
	size_t headers_buffer_size;
	size_t body_buffer_size;

	char * method;
	char * uri;
	char * host;
	char * accept;
	char * connection;
	char * cookie;

} elix_http_request;

#ifdef __cplusplus
extern "C" {
#endif

void elix_ucstring_copy( const unsigned char * source_init, char * dest_init, size_t dest_size);

elix_http_request elix_http_request_parse(elix_allocated_buffer * buffer, bool header_copy, bool body_copy);

#ifdef __cplusplus
}
#endif

int64_t elix_cstring_convert_to_int(const char * str);










#endif // ELIX_EXTRA_HPP