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

#ifndef ELIX_HTML_HEADER
#define ELIX_HTML_HEADER

#include "elix_core.h"
#include "elix_parse.h"


//Printf helpers macros
#define EHSP(a) (int)a.length, a.string
#define EHSPp(a) (int)a->length, a->string
#define pEHSP "%.*s"

enum elix_html_element_type {
	ELEMENT_FOREIGN,
	ELEMENT_VOID,
	ELEMENT_TEMPLATE,
	ELEMENT_RAWTEXT,
	ELEMENT_RAWTEXTAREA,
	ELEMENT_NORMAL
};

#define ELIX_HTML_LIST_POOL_SIZE 8

typedef struct elix_html_listpool {
	uint8_t used;
	data_pointer values[8];
} elix_html_listpool;

typedef struct elix_html_nodelist elix_html_nodelist;
typedef struct elix_html_attrlist elix_html_attrlist;

typedef struct elix_html_nodelist {
	elix_html_listpool active;
	elix_html_nodelist * next;
} elix_html_nodelist;

typedef struct elix_html_attrlist {
	elix_html_listpool active;
	elix_html_attrlist * next;
} elix_html_attrlist;

struct elix_html_node_object;
typedef struct elix_html_node_object* elix_html_node;

struct elix_html_attr_object;
typedef struct elix_html_attr_object* elix_html_attr;


typedef struct elix_html_document {
	elix_html_nodelist all_nodes;
	elix_string_buffer * reference;
	elix_html_node root;
} elix_html_document;


typedef struct elix_html_attr_object {
	elix_string_pointer name;
	elix_string_pointer value;
} elix_html_attr_object;

typedef struct elix_html_node_object {
	uint8_t type;
	char name[16];
	elix_string_pointer source;
	elix_string_pointer textContent;
	elix_html_node parent;
	elix_html_nodelist children;
	elix_html_attrlist attribute;
	
} elix_html_node_object;

#ifdef __cplusplus
extern "C" {
#endif

elix_html_document * elix_html_open( elix_string_buffer * content);
elix_parse_status elix_html_parse(elix_html_document * doc, elix_parse_status * lastStatus);
void elix_html_print(elix_html_document * doc);
void elix_html_close(elix_html_document * doc);

elix_html_node elix_html_get_node_by_tag(elix_html_node parent_node, const char * name);

#ifdef __cplusplus
}
#endif


#endif // ELIX_HTML_HEADER

