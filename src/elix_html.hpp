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

#ifndef ELIX_HTML_HPP
#define ELIX_HTML_HPP

#include "elix_core.h"
#include <memory>
#include <vector>



struct elix_htmlstring_pointer {
	elix_string_buffer * source;
	uint8_t * string;
	size_t offset;
	size_t length;
};




struct elix_character {
	uint32_t value;
	uint8_t bytes;
	uint8_t padding;
	uint16_t codepage; //0 = ASCII, elsewise Unicode
};

struct elix_html_node_object;
typedef std::shared_ptr<elix_html_node_object> elix_html_node;

struct elix_html_attr {
	elix_htmlstring_pointer name;
	elix_htmlstring_pointer value;
};
struct elix_html_node_object {
	uint8_t type = 0;
	char name[16];
	elix_htmlstring_pointer source;
	elix_html_node parent;
	std::vector<elix_html_node> children;
	std::vector<elix_html_attr> attribute;
	elix_htmlstring_pointer textContent;
};


struct elix_html_status {
	size_t offset = 0;
	size_t length = 0;
};


struct elix_html_document {
	std::vector<elix_html_node> nodes;
	elix_string_buffer * reference;
	elix_html_node root;
};

elix_html_document elix_html_open(elix_string_buffer & data);
elix_html_status elix_html_parse(elix_html_document & doc, elix_html_status * lastStatus = nullptr);
void elix_html_print(elix_html_document * doc);
void elix_html_close(elix_html_document * doc);


#endif // ELIX_HTML_HPP

