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

#ifndef ELIX_HTTP_HEADER
#define ELIX_HTTP_HEADER

#include "elix_core.h"
#include "elix_parse.h"

typedef struct elix_http_request_ref {
	elix_string * source;
	elix_string_pointer header;
	elix_string_pointer content;


	elix_string_pointer method;
	elix_string_pointer uri;
	elix_string_pointer host;
	elix_string_pointer accept;
	elix_string_pointer connection;
	elix_string_pointer cookie;
	elix_string_pointer authorization;
} elix_http_request_ref;

typedef struct elix_http_response_ref {
	elix_string * source;
	elix_string_pointer header;
	elix_string_pointer content;
	
	elix_string_pointer date;
	elix_string_pointer etag;
	elix_string_pointer content_type;
	elix_string_pointer modified;
	elix_string_pointer content_encoding;
	elix_string_pointer cookie;
	elix_string_pointer length;

} elix_http_response_ref;


#endif // ELIX_HTTP_HEADER