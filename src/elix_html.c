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

#include "elix_html.h"

/*
root
- Element
- @attribute
- @children
*/


typedef enum elix_html_parse_state { PARSE_NONE = 0, PARSE_TAG,PARSE_START_TAG,PARSE_END_TAG, PARSE_ATTRIBUTE_KEY, PARSE_ATTRIBUTE, PARSE_COMMENT, PARSE_SCAN_DOCTYPE, PARSE_SCAN_CDATA, PARSE_SCAN_COMMENT, PARSE_SCAN_TEXT, PARSE_ERROR = 0xFF} elix_html_parse_state;
static const char * elix_html_parse_state_names[] = {"PARSE_NONE", "PARSE_TAG","PARSE_START_TAG","PARSE_END_TAG", "PARSE_ATTRIBUTE_KEY", "PARSE_ATTRIBUTE", "PARSE_COMMENT", "PARSE_SCAN_DOCTYPE", "PARSE_SCAN_CDATA", "PARSE_SCAN_COMMENT", "PARSE_SCAN_TEXT", "PARSE_ERROR"};

#define PARSE_NAME(s) (s <= PARSE_SCAN_TEXT && s >= PARSE_NONE ? elix_html_parse_state_names[s] : elix_html_parse_state_names[10])


typedef struct elix_html_string_match {
	uint32_t string[7];
	union {
		uint8_t size;
		uint32_t size32;
	};
	elix_html_parse_state result;
} elix_html_string_match;

typedef struct elix_html_parse_next_info {
	elix_html_parse_state state;
	uint8_t index;
	uint8_t padding[3];
} elix_html_parse_next_info;

static elix_html_string_match doctype_start = { {'d','o','c','t','y','p','e'}, {7}, PARSE_SCAN_DOCTYPE };
static elix_html_string_match comment_start = { {'-','-'}, {2}, PARSE_SCAN_COMMENT };
static elix_html_string_match comment_end = { {'-','-', '>'}, {3}, PARSE_NONE };
static elix_html_string_match cdata_start = { {'[','C','D','A','T','A','['}, {7}, PARSE_SCAN_CDATA };
static elix_html_string_match cdata_end = { {']',']','>'}, {3}, PARSE_NONE };

#define STATECHANGE(Q, D) Q;// printf("%*s]" #Q "\n", D, " ");


bool doesStringMatch(uint32_t c, elix_html_parse_next_info info, elix_html_string_match needle, elix_html_parse_state * results) {
	if ( info.index >= needle.size) {
		return false;
	} else if ( c == needle.string[info.index] ) {
		if ( *results == PARSE_NONE ) {
			*results = needle.result;
		} else if ( *results != needle.result ) {
			//printf("doesStringMatch mode switched %s to %s\n", PARSE_NAME(results), PARSE_NAME(needle.result));
		}
		if ( info.index ==  needle.size -1 ) {
			*results = needle.result;
		}
		//printf("doesStringMatch %d\n ", results);
		return true;
	} else {
		return false;
	}
}

bool doesInsensitiveStringMatch(uint32_t c, elix_html_parse_next_info info, elix_html_string_match needle, elix_html_parse_state * results) {
	return doesStringMatch(lowerCaseASCII(c), info, needle, results);
}


void elix_html_nodelist_push(elix_html_nodelist * nl, elix_html_node node) {
	if ( !nl || !nl->active.used) {
		LOG_ERROR("Invalid elix_html_nodelist");
		return;
	}
	if ( nl->active.used < 8 ) {
		nl->active.values[nl->active.used && 0x07] = node;
		nl->active.used++;
		return;
	}
	if ( !nl->next ) {
		nl->next = ALLOCATE(elix_html_nodelist, 1);
	}
	if ( !nl->next ) {
		LOG_MESSAGE("Can not create more elix_html_nodelist");
		return;
	}
	elix_html_nodelist_push(nl->next, node);
}

void elix_html_attrlist_push(elix_html_attrlist * nl, elix_html_attr node) {
	if ( !nl || !nl->active.used) {
		LOG_ERROR("Invalid elix_html_attrlist_push");
		return;
	}
	if ( nl->active.used < 8 ) {
		nl->active.values[nl->active.used && 0x07] = node;
		nl->active.used++;
		return;
	}
	if ( !nl->next ) {
		nl->next = ALLOCATE(elix_html_attrlist, 1);
	}
	if ( !nl->next ) {
		LOG_MESSAGE("Can not create more elix_html_attrlist_push");
		return;
	}
	elix_html_attrlist_push(nl->next, node);
}

elix_html_attr elix_html_attrlist_get_last(elix_html_attrlist * nl) {
	if (!nl || !nl->active.used) {
		LOG_ERROR("Invalid elix_html_attrlist");
		return nullptr;
	}
	if (nl->active.used > 0) {
		return nl->active.values[(nl->active.used - 1) & 0x07];
	}
	if (nl->next) {
		return elix_html_attrlist_get_last(nl->next);
	}
	return nullptr;
}

elix_html_node elix_html_nodelist_get(elix_html_nodelist * nl, uint16_t index) {
	if (!nl || !nl->active.used) {
		LOG_ERROR("Invalid elix_html_attrlist");
		return nullptr;
	}
	if (nl->active.used > index) {
		return (elix_html_node)nl->active.values[(index) & 0x07];
	}
	if (nl->next) {
		return elix_html_nodelist_get(nl->next, index - 8);
	}
	return nullptr;
}

elix_html_node elix_html_nodelist_get_last(elix_html_nodelist * nl) {
	if (!nl || !nl->active.used) {
		LOG_ERROR("Invalid elix_html_attrlist");
		return nullptr;
	}
	if (nl->active.used > 0) {
		return nl->active.values[(nl->active.used - 1) & 0x07];
	}
	if (nl->next) {
		return elix_html_nodelist_get_last(nl->next);
	}
	return nullptr;
}

elix_html_node elix_html_node_push(elix_html_node current_node, uint8_t type, elix_html_document * doc) {
	elix_html_node next_node;
	if ( current_node == nullptr ) {
		next_node = doc->root;
	} else {
		next_node = ALLOCATE(elix_html_node_object, 1);
		next_node->type = type;
		next_node->parent = current_node;
		elix_html_nodelist_push( &doc->all_nodes, next_node);
		elix_html_nodelist_push( &current_node->children, next_node);
	}
	return next_node;
}

elix_html_node elix_html_node_push_text(elix_html_node current_node, elix_html_document * doc, elix_parse_status * current, size_t * text_length) {
	elix_html_node next_node = elix_html_node_push(current_node, ELEMENT_RAWTEXT, doc);
	memcpy(next_node->name, "TextNode", 9);
	next_node->textContent = elix_string_buffer_get_pointer(doc->reference, current->offset - *text_length, *text_length);
	*text_length = 0;
	return next_node;
}


elix_html_node elix_html_node_push_element(elix_html_node current_node, elix_html_document * doc, elix_string * buffer) {
	elix_html_node next_node = elix_html_node_push(current_node, ELEMENT_NORMAL, doc);
	memcpy(next_node->name, buffer->text, 16);

	return next_node;
}

elix_html_node elix_html_node_push_void(elix_html_node current_node, elix_html_document * doc, elix_string * buffer, elix_parse_status * current) {
	elix_html_node next_node = elix_html_node_push(current_node, ELEMENT_VOID, doc);
	next_node->textContent = elix_string_buffer_get_pointer(doc->reference, current->offset - 2 - buffer->length, buffer->length);
	return next_node;
}

bool elix_htmlstring_compare(elix_string_pointer primary, elix_string_pointer secondary) {
	if (primary.length != secondary.length) {
		return false;
	}
	return strncmp((const char*)primary.string, (const char*)secondary.string, primary.length) == 0;
}


bool elix_html_node_name_is(elix_html_node node, const char * name) {
	ASSERT(node != nullptr);
	for (size_t c = 0; node->name[c] && name[c]; c++ ) {
		if ( node->name[c] != name[c] )
			return false;
	}
	return true;
}

elix_html_node elix_html_get_node_by_tag_from_list(elix_html_nodelist * node_list, const char * name) {
	for ( uint8_t c = 0; c < node_list->active.used; c++) {
		if ( elix_html_node_name_is(node_list->active.values[c], name) ) {
			return node_list->active.values[c];
		}
	}
	if ( node_list->next ) {
		return elix_html_get_node_by_tag_from_list(node_list->next, name);
	}
	return nullptr;
}

elix_html_node elix_html_get_node_by_tag(elix_html_node parent_node, const char * name) {
	return elix_html_get_node_by_tag_from_list(&parent_node->children, name);
	
}

elix_parse_status elix_html_parse(elix_html_document * doc, elix_parse_status * lastStatus) {
	bool debug = false;
	elix_html_node current_node;
	elix_parse_status current;

	current.length = doc->reference->length;
	if ( lastStatus != nullptr ) {
		//TODO: Continue parsing from a point
		if ( lastStatus->offset > current.length ) {
			//Pasted the end of file
			return current;
		}
		if ( lastStatus->length > current.length ) {
			//File Changed
		}
	}

	if ( current.length < 5 ) {
		return current;
	}

	elix_html_parse_state state = PARSE_NONE;
	elix_string buffer = elix_string_new(1024);

	elix_character char32;

    //Set up pointer to current character
    doc->reference->iter = doc->reference->data + current.offset;

	int d = 0;
	size_t text_length = 0;
	elix_html_parse_next_info reset_parse_next_info;
	elix_html_parse_next_info comment_next;
	elix_html_parse_next_info scan_next;
	uint32_t counter = 0;
	do {
		char32 = elix_character_next(doc->reference);
		if ( char32.value > 0xFF) {
			int q = 0;
		}

		if ( char32.value ) {
			switch (state) {
				case PARSE_TAG: {
					if ( isVoidTagChar(char32.value) ) {
						/* / */ 
						state = STATECHANGE(PARSE_END_TAG, d);
					} else if ( isValidNameChar(char32.value, !buffer.length) ) {
						/* a-z, if not first 0-9 also */
						state = STATECHANGE(PARSE_START_TAG, d);
						elix_string_append( &buffer, char32.value);
					} else if ( char32.value == '!') {
						/* Comment <! >*/
						state = STATECHANGE(PARSE_COMMENT, d);
						comment_next = reset_parse_next_info;
					} else if ( char32.value == '?') {
						/* HACK: <?xml version="1.0"?> */
						state = STATECHANGE(PARSE_SCAN_DOCTYPE, d);
						comment_next = reset_parse_next_info;
					} else {
						/* Namespaces not supported*/
						state = STATECHANGE(PARSE_ERROR, d);
						printf("Error Parsing Tag at char " pZU "\n", current.offset);
						return current;
					}
					break;
				}
				case PARSE_START_TAG: {
					if ( isValidNameChar(char32.value, 0) ) {
						/* a-z or 0-9 */
						state = STATECHANGE(PARSE_START_TAG, d);
						elix_string_append( &buffer, char32.value);
					} else if ( char32.value == ' ' ) {
						/* If space switch to attributes*/
						current_node = elix_html_node_push_element(current_node, doc, &buffer);
						state = STATECHANGE(PARSE_ATTRIBUTE_KEY, d);
						elix_string_clear(&buffer);
					} else if ( char32.value == '>' ) {
						// finish opening tag
						current_node = elix_html_node_push_element(current_node, doc, &buffer);
						state = STATECHANGE(PARSE_NONE, d);
						d++;
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						printf("Error Parsing Tag at char " pZU "\n", current.offset);
						return current;
					}
					break;
				}
				case PARSE_SCAN_DOCTYPE: {
					//TODO check if > is not in quotes
					if ( char32.value == '>' ) {
						state = STATECHANGE(PARSE_NONE, d);
						printf("Doctype '%*s'\n", buffer.length, buffer.text);
						elix_string_clear(&buffer);
					} else {
						elix_string_append( &buffer, char32.value );
					}
					break;
				}
				case PARSE_SCAN_COMMENT: {
					bool f = doesStringMatch(char32.value, scan_next, comment_end, &state);
					if ( f ) {
						//if comment_end is found, return to parse to none
						if ( state == PARSE_SCAN_COMMENT ) {
							scan_next.index++;
						} else {
							current_node = elix_html_node_push_void(current_node, doc, &buffer, &current);
							current_node = current_node->parent;
							state = STATECHANGE(PARSE_NONE, d);
						}
					} else {
						if ( scan_next.index ) {
							//else if comment_end is semi found and scan_index > 0, append to buffer. parse stays the same
							for (uint8_t i = 0; i < scan_next.index && i < comment_end.size; i++) {
								elix_string_append( &buffer, comment_end.string[i] );
							}
							scan_next.index = 0;
						}
						//else add to buffer
						elix_string_append( &buffer, char32.value );
					}
					break;
				}
				case PARSE_SCAN_CDATA: {
					bool f = doesStringMatch(char32.value, scan_next, cdata_end, &state);
					if ( f ) {
						//if comment_end is found, return to parse to none
						if ( state == PARSE_SCAN_CDATA ) {
							scan_next.index++;
						} else {
							current_node = elix_html_node_push_void(current_node, doc, &buffer, &current);
							current_node = current_node->parent;
							state = STATECHANGE(PARSE_NONE, d);
						}
					} else {
						if ( scan_next.index ) {
							//else if comment_end is semi found and scan_index > 0, append to buffer. parse stays the same
							for (uint8_t i = 0; i < scan_next.index && i < cdata_end.size; i++) {
								elix_string_append( &buffer, cdata_end.string[i] );
							}
							scan_next.index = 0;
						}

						//else add to buffer
						elix_string_append( &buffer, char32.value );
					}
					break;
				}
				case PARSE_COMMENT: {
					//<!doctype				>
					//<!--					-->
					//<![CDATA[				]]>

					if (comment_next.state == PARSE_NONE) { //First Char
						if ( doesInsensitiveStringMatch(char32.value, comment_next, doctype_start, &comment_next.state) ) {
						} else if ( doesStringMatch(char32.value, comment_next, comment_start, &comment_next.state ) ) {
						} else if ( doesStringMatch(char32.value, comment_next, cdata_start, &comment_next.state)) {
						} else {
							state = STATECHANGE(PARSE_ERROR, d);
							LOG_MESSAGE("Error Parsing <! Tag at char " pZU "\n%.*s\n%*c^\n", current.offset, (current.offset < 4 ? 0 : current.offset - 4), doc->reference->iter, current.offset < 4 ? current.offset-1 : 3, ' ' );
							return current;
						}
					} else if ( comment_next.state == PARSE_SCAN_COMMENT && doesStringMatch(char32.value, comment_next, comment_start, &state) ) {
						scan_next = reset_parse_next_info;
					} else if ( comment_next.state == PARSE_SCAN_DOCTYPE && doesInsensitiveStringMatch(char32.value, comment_next, doctype_start, &state) ) {
						scan_next = reset_parse_next_info;
					} else if ( comment_next.state == PARSE_SCAN_CDATA && doesStringMatch(char32.value, comment_next, cdata_start, &state) ) {
						scan_next = reset_parse_next_info;
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						LOG_MESSAGE("Error Parsing <! Tag at char " pZD "\n%.*s\n%*c^\n", current.offset, current.offset < 4 ? 0 : current.offset - 4, doc->reference->iter, current.offset < 4 ? current.offset-1 : 3, '-' );
						return current;
					}
					comment_next.index++;
					break;
				}
				case PARSE_END_TAG: {
					if ( isValidNameChar(char32.value, 0) ) {
						state = STATECHANGE(PARSE_END_TAG, d);
						elix_string_append( &buffer, char32.value );
					} else if ( char32.value == '>' ) {
						state = STATECHANGE(PARSE_NONE, d);

						current_node->textContent.length = buffer.length;

						if (current_node->children.active.used == 1 ) {
						
							if (((elix_html_node)current_node->children.active.values[0])->type == ELEMENT_RAWTEXT)
								current_node->textContent = ((elix_html_node)current_node->children.active.values[0])->textContent;
						}
						current_node = current_node->parent;
						
						elix_string_clear(&buffer);
						d--;
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						LOG_MESSAGE("Error Parsing Tag at char " pZD "\n%.*s\n%*c^\n", current.offset, current.offset < 4 ? 0 : current.offset - 4, doc->reference->iter, current.offset < 4 ? current.offset-1 : 3, '-' );
						return current;
					}
					break;
				}
				case PARSE_SCAN_TEXT: {
					if ( isOpenTagChar(char32.value) ) {
						current_node = elix_html_node_push_text(current_node, doc, &current, &text_length);
						current_node = current_node->parent;
						state = STATECHANGE(PARSE_TAG, d);
						elix_string_clear(&buffer);
					} else {
						text_length += char32.bytes;
					}					
					break;
				}
				case PARSE_ATTRIBUTE_KEY: {
					/*
					 if isValidNameChar set as name, if > name & tag if finished. = switch to valve scan
					*/
					if ( char32.value == '>' ) {
						//close attr
						if ( buffer.length ) {
							elix_html_attr new_attribute = {0};
							new_attribute->name = elix_string_buffer_get_pointer(doc->reference, current.offset - buffer.length, buffer.length);
							elix_html_attrlist_push(&current_node->attribute, new_attribute);

							if ( debug )
								printf("[>] name:'%.*s'\n", new_attribute->name.length, new_attribute->name.string);
							elix_string_clear(&buffer);
						}
						state = STATECHANGE(PARSE_NONE, d);
					} else if ( isValidNameChar(char32.value, !buffer.length) ) {
						elix_string_append( &buffer, char32.value );
					} else if ( char32.value == ' ' ) {
						///NOTE:  
						//If buffer has content, space ends it.
						if ( buffer.length ) {
							elix_html_attr new_attribute = {0};
							new_attribute->name = elix_string_buffer_get_pointer(doc->reference, current.offset - buffer.length, buffer.length);
							elix_html_attrlist_push(&current_node->attribute, new_attribute);

							if ( debug )
								printf("[ ] name:'%.*s'\n", new_attribute->name.length, new_attribute->name.string);
							elix_string_clear(&buffer);
						}
					} else if ( char32.value == '=' ) {
						elix_html_attr new_attribute = {0};
						new_attribute->name = elix_string_buffer_get_pointer(doc->reference, current.offset - buffer.length, buffer.length);
						elix_html_attrlist_push(&current_node->attribute, new_attribute);

						if ( debug )
							printf("[=] name:'%.*s' %s\n", new_attribute->name.length, new_attribute->name.string, buffer.text);
						elix_string_clear(&buffer);
						state = STATECHANGE(PARSE_ATTRIBUTE, d);
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						LOG_MESSAGE("Error Parsing Attribute at char " pZD "\n%.*s\n%*c^\n", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
						return current;
					}					
					break;
				}
				case PARSE_ATTRIBUTE: {
					//Get current attribute
					if ( char32.value == '"' ) {
						// Start or end value
						if ( buffer.length >= 3 ) {
							elix_html_attr new_attribute = elix_html_attrlist_get_last(&current_node->attribute);
							//Buffer contains quotes, so we change buffer length by 2
							new_attribute->value = elix_string_buffer_get_pointer(doc->reference, current.offset - buffer.length + 1, buffer.length - 1);
							if (debug)
								printf("[+] value:'%.*s' %s\n", (int)new_attribute->value.length, new_attribute->value.string, buffer.text);
							elix_string_clear(&buffer);
							state = STATECHANGE(PARSE_ATTRIBUTE_KEY, d);
						} else if ( buffer.length ) {
							//Buffer length should be atleast 2 to include the quote marks, if not then something went wrong
							state = STATECHANGE(PARSE_ERROR, d);
							LOG_MESSAGE("Error Parsing Attribute at char " pZD "\n%.*s\n%*c^\n", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
							return current;
						} else {
							elix_string_append( &buffer, char32.value );
						}
					} else if ( buffer.length ) {
						elix_string_append( &buffer, char32.value );
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						LOG_MESSAGE("Error Parsing Attribute at char " pZD "\n%.*s\n%*c^\n", current.offset, current.offset < 4 ? 0 : (int)current.offset - 4, doc->reference->iter, current.offset < 4 ? (int)current.offset-1 : 3, '-' );
						return current;
					}							
					break;
				}
				default: {
					if ( isOpenTagChar(char32.value) ) {
						state = STATECHANGE(PARSE_TAG, d);
						elix_string_clear(&buffer);
					} else {
						state = STATECHANGE(PARSE_SCAN_TEXT, d);
						elix_string_clear(&buffer);
						text_length += char32.bytes;
					}
					break;
				}
			}
		}
		current.offset += char32.bytes;
	} while (char32.value && current.offset < current.length );

	return current;
}

elix_html_document * elix_html_open( elix_string_buffer * content) {
	elix_parse_status status = {};
	elix_html_document * doc = ALLOCATE(elix_html_document, 1);
	doc->root = ALLOCATE(elix_html_node_object, 1);

	size_t first_character = 0;

	while ( first_character < content->length && isWhiteSpace(content->data[first_character]) ) {
	    first_character++;
	}

    if ( content->data[first_character] == '<' ) {
        ///NOTE: Reference may be freed elsewhere
		doc->reference = content;
	} else {
		LOG_ERROR("Error: HTML pages must start <\n");
	}
	elix_html_parse(doc, &status);
	return doc;
}

void elix_html_printNode(elix_html_node * node, size_t * d) {
	ASSERT(node);
	/*
	for (size_t var = 0; var < d; ++var) {
		printf(" ");
	}
	elix_html_node_object * obj = node->get();

	//ELEMENT_FOREIGN,
	//ELEMENT_VOID,
	//ELEMENT_TEMPLATE,
	//ELEMENT_RAWTEXT,
	//ELEMENT_RAWTEXTAREA,
	//ELEMENT_NORMAL


	switch (obj->type) {
		case ELEMENT_RAWTEXT:
			if (obj->textContent.length)
				printf("(TEXT)'%.*s'\n", (int)obj->textContent.length, obj->textContent.string);
			break;
		case ELEMENT_RAWTEXTAREA:
			if (obj->textContent.length)
				printf(">> '%.*s'\n", (int)obj->textContent.length, obj->textContent.string);
			break;
		case ELEMENT_VOID:
			if (obj->textContent.length)
				printf("(ELEMENT_VOID) '%.*s'\n", (int)obj->textContent.length, obj->textContent.string);
			break;
		case ELEMENT_NORMAL:
			if ( !obj->attribute.empty() ) {
				printf("[%s", obj->name);
				for (elix_html_attr a: obj->attribute) {
					if ( a.value.length ) {
						printf(" (%.*s", (int)a.name.length, a.name.string);
						printf(":%.*s)", (int)a.value.length, a.value.string);
					} else {
						printf(" %.*s", (int)a.name.length, a.name.string);
					}
				}
				printf("]\n");
			} else {
				printf("[%s]\n", obj->name);
			}
			break;
		case ELEMENT_FOREIGN:
			printf("(ELEMENT_FOREIGN:%s)\n", obj->name);
			break;
		case ELEMENT_TEMPLATE:
			printf("(ELEMENT_TEMPLATE:%s)\n", obj->name);
			break;
		default:
			printf("(:%s)\n", obj->name);
			break;
	}

	for (elix_html_node q: node->get()->children) {
		d++;
		elix_html_printNode(&q, d);
		d--;
	}
		*/
}


void elix_html_print(elix_html_document * doc) {
	size_t d = 0;

	if (doc->root->type) {
		elix_html_printNode(&doc->root, &d);
	} else {
		printf("No Root Node found\n");
	}
}
/*
	void attached_rendertree_item(elix_html_node_object * node, elix_rendertree_item * item) {
		node->render_item.children.insert(node->render_item.children.end(), 1, item);
	}

	void deattached_rendertree_item(elix_html_node_object * node, elix_rendertree_item * item) {
		node->render_item.children.insert(node->render_item.children.end(), 1, item);
	}


	void update_rendertree_item(elix_html_document * doc, elix_html_node &node, elix_rendertree_item *& item) {
		node_object * obj = node.get();
		switch (obj->type) {
			case ELEMENT_RAWTEXT:
				item->render_style.backgroundColour.hex = 0x000000FF;
				item->data = &obj->textContent;
				item->data_type = ERTD_STRING;
				item->render_style.display = ERT_INLINEBLOCK;
				break;
			case ELEMENT_NORMAL:
				item->data_type = ERTD_EMPTY;
				item->render_style.display = ERT_BLOCK;

				//item->data = &obj->textContent;
				break;
			default:
				item->data_type = ERTD_EMPTY;
				break;
		}

		for (elix_rendertree_item *  q: item->children) {


		}
	}

	void update_render_tree(elix_html_node * node, elix_rendertree & tree) {

	}

	uint8_t build_rendertree_item(elix_html_document * doc, elix_html_node &node, elix_rendertree_item * parent) {
		node_object * obj = node.get();
		if (obj->type) {
			switch (obj->type) {
				case ELEMENT_RAWTEXT:
					obj->render_item.render_style.backgroundColour.hex = 0x000000FF;
					obj->render_item.data = &obj->textContent;
					obj->render_item.data_type = ERTD_STRING;
					obj->render_item.render_style.display = ERT_INLINEBLOCK;
					printf("ELEMENT_RAWTEXT\n");
					break;
				case ELEMENT_NORMAL:
					obj->render_item.data_type = ERTD_EMPTY;
					obj->render_item.render_style.display = ERT_BLOCK;
					if ( parent ) {
						obj->render_item.render_style.backgroundColour.hex = 0xFFFFFFFF;
						obj->render_item.render_style.width = parent->render_style.width;
					} else  {
						obj->render_item.render_style.backgroundColour.hex = 0xFF0000FF;
						obj->render_item.render_style.width = doc->rendertree.width;
						obj->render_item.render_style.height = doc->rendertree.height;
					}
					printf("ELEMENT_NORMAL\n");
					break;
				default:
					obj->render_item.data_type = ERTD_EMPTY;
					break;
			}

			obj->render_item.children.clear();

			for (elix_html_node q: obj->children) {
				elix_html_node_object * q_obj = q.get();
				build_rendertree_item(doc, q, &obj->render_item);
				obj->render_item.children.push_back(&q.get()->render_item);
			}

			return 0;
		}

		return 1;
	}







	void clear_rendertree_item(elix_rendertree_item *& item) {
		for (elix_rendertree_item * q: item->children) {
			clear_rendertree_item(q);
		}
		item->children.clear();
		delete item;

	}


	void clear_render_tree(elix_html_document * doc) {
		if (doc->root.get()->type) {
			clear_rendertree_item(doc->rendertree.root);
		} else {
			printf("No Root Node found\n");
		}


	}

	elix_rendertree build_render_tree(elix_html_document * doc, elix_uv32_2 dimension) {

		if (doc->root.get()->type) {
			doc->rendertree.width = dimension.width;
			doc->rendertree.height = dimension.height;
			build_rendertree_item(doc, doc->root, nullptr);
		} else {
			printf("No Root Node found\n");
		}

		return doc->rendertree;
	}

*/

