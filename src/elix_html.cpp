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

#include "elix_html.hpp"

#include <locale>
#include <cstring>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

elix_string elix_string_new(uint16_t allocate) {
    elix_string str;

    if ( allocate ) {
        str.text = (uint8_t*)malloc(allocate);
        str.allocated = allocate;
    }
    return str;
}

void elix_string_clear(elix_string & str) {
	if ( str.owned ) {
		memset(str.text, 1, str.allocated);
		str.length = 0;
	}
}

void elix_string_append_byte(elix_string & str, uint8_t byte ) {
	//resize string
	if ( str.length < str.allocated - 1 ) {
		void * newptr = realloc(str.text, str.allocated + 8 );
		if ( !newptr ) {
			LOG_ERROR("String couldn't be resized");
			return;
		}
		str.text = (uint8_t*)newptr;
		str.allocated += 8;
	}

	str.text[str.length] = byte;
	str.length++;
}

void elix_string_append(elix_string & str, uint32_t char32 ) {

	uint8_t char8 = 0;
	if ( char32 <= 0x7F ) {
		char8 = char32 & 0x7F;
		elix_string_append_byte(str, char8);
	} else if (char32 <= 0x07FF) {
		char8 = ( (char32 >> 6) & 0x1F) | 0xC0;
		elix_string_append_byte(str, char8);
		char8 = (char32 & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
	} else if (char32 <= 0xFFFF) {
		char8 = ( (char32 >> 12) & 0x0F) | 0xE0;
		elix_string_append_byte(str, char8);
		char8 = ( (char32 >> 6) & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
		char8 = (char32 & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
	} else if (char32 <= 0x10FFFF) {
		char8 = ( (char32 >> 18) & 0x07) | 0xF0;
		elix_string_append_byte(str, char8);

		char8 = ( (char32 >> 12) & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
		char8 = ( (char32 >> 6) & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
		char8 = (char32 & 0x3F) | 0x80;
		elix_string_append_byte(str, char8);
	} else {
		elix_string_append_byte(str, 0xEF);
		elix_string_append_byte(str, 0xBF);
		elix_string_append_byte(str, 0xBD);
	}
}

void elix_string_buffer_forward(elix_string_buffer * str ) {
    str->location++;
    if ( str->iter ) {
    	str->iter++;
    } else {
	    str->iter = str->data + str->location;
    }

}

inline elix_htmlstring_pointer elix_string_buffer_get_pointer( elix_string_buffer * buffer, size_t offset, size_t length ) {
    elix_htmlstring_pointer str;

	if ( offset + length >= buffer->length ) {
		return str;
	}

	str.source = buffer;
	str.string = buffer->data + offset;
	str.offset = offset;
	str.length = length;

    return str;
}

elix_character elix_character_next( elix_string_buffer * text )
{
	elix_character char32 = {0,1,0,1};

	uint8_t char8 = 0;
	uint32_t next;

	char8 = (*text->iter);
	if ( char8 < 128 ) {
		char32.value = char8;
		char32.codepage = 0; //ASCII
	} else if ( char8 > 193 && char8 < 224 ) {
		elix_string_buffer_forward(text);
		next = (*text->iter);

		char32.value = ((char8 << 6) & 0x7ff) + (next & 0x3f);
	} else if ( char8 > 223 && char8 < 240 ) {
		elix_string_buffer_forward(text);
		next = (*text->iter) & 0xff;
		char32.value = ((char8 << 12) & 0xffff) + ((next << 6) & 0xfff);

		elix_string_buffer_forward(text);
		next = (*text->iter) & 0x3f;
		char32.value += next;
	} else if ( char8 > 239 && char8 < 245 ) {
		elix_string_buffer_forward(text);
		next = (*text->iter) & 0xff;
		char32.value = ((char8 << 18) & 0xffff) + ((next << 12) & 0x3ffff);

		elix_string_buffer_forward(text);
		next = (*text->iter) & 0xff;
		char32.value += (next << 6) & 0xfff;

		elix_string_buffer_forward(text);
		next = (*text->iter) & 0x3f;
		char32.value += next;

	} else {
		char32.value = 0xFFFD;
	}
	elix_string_buffer_forward(text);
	return char32;
}


/*
root
- Element
- @attribute
- @children
*/

enum elix_html_element_type {
	ELEMENT_FOREIGN,
	ELEMENT_VOID,
	ELEMENT_TEMPLATE,
	ELEMENT_RAWTEXT,
	ELEMENT_RAWTEXTAREA,
	ELEMENT_NORMAL
};
enum elix_html_parse_state { PARSE_NONE, PARSE_TAG,PARSE_START_TAG,PARSE_END_TAG, PARSE_ATTRIBUTE, PARSE_COMMENT, PARSE_SCAN_DOCTYPE, PARSE_SCAN_CDATA, PARSE_SCAN_COMMENT, PARSE_SCAN_TEXT, PARSE_ERROR = 0xFF};

struct elix_string_match {
	uint32_t string[7];
	union {
		uint8_t size;
		uint32_t size32;
	};
	elix_html_parse_state result;
};

struct elix_html_parse_next_info {
	elix_html_parse_state state = PARSE_NONE;
	uint8_t index = 0;
	uint8_t padding[3] = {0};
};

static elix_string_match doctype_start = { {'d','o','c','t','y','p','e'}, {7}, PARSE_SCAN_DOCTYPE };
static elix_string_match comment_start = { {'-','-'}, {2}, PARSE_SCAN_COMMENT };
static elix_string_match comment_end = { {'-','-', '>'}, {3}, PARSE_NONE };
static elix_string_match cdata_start = { {'[','C','D','A','T','A','['}, {7}, PARSE_SCAN_CDATA };
static elix_string_match cdata_end = { {']',']','>'}, {3}, PARSE_NONE };

#define STATECHANGE(Q, D) Q;// printf("%d: " #Q "\n", D);
inline bool isOpenTagChar(uint32_t c) {
	return ( c == '<');
}
inline bool isEndTagChar(uint32_t c) {
	return ( c == '>');
}
inline bool isVoidTagChar(uint32_t c) {
	return ( c == '/');
}

inline bool isWhiteSpace(uint32_t c) {
    return ( c == ' ' || c == '\t' || c == '\n' );
}

inline bool isValidNameChar(uint32_t c, uint8_t first) {
	if ( ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
		return true;
	} else if ( !first && ((c >= '0' && c <= '9') || (c == '-'))) {
		return true;
	} else {
		return false;
	}
}
inline uint32_t lowerCaseASCII(uint32_t c) {
	if ( (c >= 'A' && c <= 'Z') ) {
		return c + 32;
	} else {
		return c;
	}
}

inline bool doesStringMatch(uint32_t c, elix_html_parse_next_info & info, elix_string_match & needle, elix_html_parse_state & results) {
	if ( info.index >= needle.size) {
		return false;
	} else if ( c == needle.string[info.index] ) {
		if ( results == PARSE_NONE ) {
			results = needle.result;
		} else if ( results != needle.result ) {
			//printf("doesStringMatch mode switched\n");
		}
		if ( info.index ==  needle.size -1 ) {
			results = needle.result;
		}
		return true;
	} else {
		return false;
	}
}

inline bool doesInsensitiveStringMatch(uint32_t c, elix_html_parse_next_info & info, elix_string_match & needle, elix_html_parse_state & results) {
	return doesStringMatch(lowerCaseASCII(c), info, needle, results);
}

inline elix_html_node elix_html_node_push(elix_html_node current_node, elix_html_document & doc) {
	elix_html_node next_node;
	if ( current_node == nullptr ) {
		next_node = doc.root;
	} else {
		next_node = std::make_shared<elix_html_node_object>();
		doc.nodes.push_back(next_node);
		next_node.get()->parent = current_node;
		current_node.get()->children.push_back(next_node);
	}
	return next_node;
}


inline elix_html_node elix_html_element_node_pop(elix_html_node current_node, elix_html_document & doc, elix_string & buffer) {
	elix_html_node next_node = elix_html_node_push(current_node,doc);
	memcpy(next_node.get()->name, buffer.text, 16);
	next_node.get()->type = ELEMENT_NORMAL;

	return next_node;
}

inline elix_html_node elix_html_void_node_pop(elix_html_node current_node, elix_html_document & doc, elix_string & buffer, elix_html_status & current) {
	elix_html_node next_node = elix_html_node_push(current_node,doc);

	elix_html_status doc_pos;
	doc_pos.offset = current.offset-2-buffer.length;
	doc_pos.length = buffer.length;
	next_node.get()->textContent = elix_string_buffer_get_pointer(doc.reference, current.offset - 2 - buffer.length, buffer.length);
	next_node.get()->type = ELEMENT_VOID;
	return next_node;
}

inline elix_html_node elix_html_node_pop(elix_html_node current_node) {
	return current_node.get()->parent;
}


elix_html_status elix_html_parse(elix_html_document & doc, elix_html_status * lastStatus) {

	elix_html_node current_node;
	elix_html_status current;

	current.length = doc.reference->length;
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
	elix_string buffer = elix_string_new(256);


	elix_character char32;

    //Set up pointer to current character
    doc.reference->iter = doc.reference->data + current.offset;

	size_t d = 0;
	elix_html_parse_next_info reset_parse_next_info;
	elix_html_parse_next_info comment_next;
	elix_html_parse_next_info scan_next;

	do {
		char32 = elix_character_next(doc.reference);
		if ( char32.value ) {
			switch (state) {
				case PARSE_TAG: {
					if ( isVoidTagChar(char32.value) ) {
						state = STATECHANGE(PARSE_END_TAG, d);
					} else if ( isValidNameChar(char32.value, !buffer.length) ) {
						state = STATECHANGE(PARSE_START_TAG, d);
						elix_string_append( buffer, char32.value);

					} else if ( char32.value == '!') {
						state = STATECHANGE(PARSE_COMMENT, d);
						comment_next = reset_parse_next_info;
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						printf("Error Parsing Tag at char " pZU "\n", current.offset);
						return current;
					}
					break;
				}
				case PARSE_START_TAG: {
					if ( isValidNameChar(char32.value, 0) ) {
						state = STATECHANGE(PARSE_START_TAG, d);
						elix_string_append( buffer, char32.value);
					} else if ( char32.value == ' ' ) {
						current_node = elix_html_element_node_pop(current_node, doc,buffer);
						state = STATECHANGE(PARSE_ATTRIBUTE, d);
					} else if ( char32.value == '>' ) {
						current_node = elix_html_element_node_pop(current_node, doc,buffer);
						state = STATECHANGE(PARSE_NONE, d);
						d++;
					} else {

					}
					break;
				}
				case PARSE_SCAN_DOCTYPE: {
					//TODO check if > is not in quotes
					if ( char32.value == '>' ) {
						state = STATECHANGE(PARSE_NONE, d);
						printf("Doctype '%*s'\n", buffer.length, buffer.text);
						elix_string_clear(buffer);
					} else {
						elix_string_append( buffer, char32.value );
					}
					break;
				}
				case PARSE_SCAN_COMMENT: {
					bool f = doesStringMatch(char32.value, scan_next, comment_end, state);
					if ( f ) {
						//if comment_end is found, return to parse to none
						if ( state == PARSE_SCAN_COMMENT ) {
							scan_next.index++;
						} else {
							current_node = elix_html_void_node_pop(current_node, doc, buffer, current);
							current_node = elix_html_node_pop(current_node);
							state = STATECHANGE(PARSE_NONE, d);
						}
					} else {
						if ( scan_next.index ) {
							//else if comment_end is semi found and scan_index > 0, append to buffer. parse stays the same
							for (uint8_t i = 0; i < scan_next.index && i < comment_end.size; i++) {
								elix_string_append( buffer, comment_end.string[i] );
							}
							scan_next.index = 0;
						}
						//else add to buffer
						elix_string_append( buffer, char32.value );
					}
					break;
				}
				case PARSE_SCAN_CDATA: {
					bool f = doesStringMatch(char32.value, scan_next, cdata_end, state);
					if ( f ) {
						//if comment_end is found, return to parse to none
						if ( state == PARSE_SCAN_CDATA ) {
							scan_next.index++;
						} else {
							current_node = elix_html_void_node_pop(current_node, doc, buffer, current);
							current_node = elix_html_node_pop(current_node);
							state = STATECHANGE(PARSE_NONE, d);
						}
					} else {
						if ( scan_next.index ) {
							//else if comment_end is semi found and scan_index > 0, append to buffer. parse stays the same
							for (uint8_t i = 0; i < scan_next.index && i < cdata_end.size; i++) {
								elix_string_append( buffer, cdata_end.string[i] );
							}
							scan_next.index = 0;
						}

						//else add to buffer
						elix_string_append( buffer, char32.value );
					}
					break;
				}
				case PARSE_COMMENT: {
					//<!doctype				>
					//<!--					-->
					//<![CDATA[				]]>

					if (comment_next.state == PARSE_NONE) { //First Char
						if ( doesInsensitiveStringMatch(char32.value, comment_next, doctype_start, comment_next.state) ) {
						} else if ( doesStringMatch(char32.value, comment_next, comment_start, comment_next.state ) ) {
						} else if ( doesStringMatch(char32.value, comment_next, cdata_start, comment_next.state)) {
						} else {
							state = STATECHANGE(PARSE_ERROR, d);
							LOG_MESSAGE("Error Parsing <! Tag at char " pZU "\n%.*s\n%*c^\n", current.offset, (current.offset < 4 ? 0 : current.offset - 4), doc.reference->iter, current.offset < 4 ? current.offset-1 : 3, ' ' );
							return current;
						}
					} else if ( comment_next.state == PARSE_SCAN_COMMENT && doesStringMatch(char32.value, comment_next, comment_start, state) ) {
						scan_next = reset_parse_next_info;
					} else if ( comment_next.state == PARSE_SCAN_DOCTYPE && doesInsensitiveStringMatch(char32.value, comment_next, doctype_start, state) ) {
						scan_next = reset_parse_next_info;
					} else if ( comment_next.state == PARSE_SCAN_CDATA && doesStringMatch(char32.value, comment_next, cdata_start, state) ) {
						scan_next = reset_parse_next_info;
					} else {
						state = STATECHANGE(PARSE_ERROR, d);
						LOG_MESSAGE("Error Parsing <! Tag at char " pZD "\n%.*s\n%*c^\n", current.offset, current.offset < 4 ? 0 : current.offset - 4, doc.reference->iter, current.offset < 4 ? current.offset-1 : 3, '-' );
						return current;
					}
					comment_next.index++;
					break;
				}
				case PARSE_END_TAG: {
					if ( isValidNameChar(char32.value, 0) ) {
						state = STATECHANGE(PARSE_END_TAG, d);
						elix_string_append( buffer, char32.value );
					} else if ( char32.value == '>' ) {
						state = STATECHANGE(PARSE_NONE, d);
						current_node.get()->textContent.length = buffer.length;
						current_node = elix_html_node_pop(current_node);
						
						elix_string_clear(buffer);
						d--;
					}
					break;
				}
				default: {
					if ( isOpenTagChar(char32.value) ) {
						if ( state == PARSE_SCAN_TEXT) {
							current_node = elix_html_node_push(current_node, doc);
							memcpy(current_node.get()->name, "TextNode", 9);
							current_node.get()->type = ELEMENT_RAWTEXT;
							current_node.get()->textContent = elix_string_buffer_get_pointer(doc.reference, doc.reference->location, 1);
							current_node = elix_html_node_pop(current_node);
						}
						state = STATECHANGE(PARSE_TAG, d);
						elix_string_clear(buffer);
					} else {
						state = STATECHANGE(PARSE_SCAN_TEXT, d);
						elix_string_append( buffer, char32.value );
					}
				}
			}
		}
		current.offset++;
	} while (char32.value && current.offset < current.length );

	return current;
}

elix_html_document elix_html_open(elix_string_buffer & content) {
	elix_html_document doc;
	doc.root = std::make_shared<elix_html_node_object>();

	size_t first_character = 0;

	while ( first_character < content.length && isWhiteSpace(content.data[first_character]) ) {
	    first_character++;
	}

    if ( content.data[first_character] == '<' ) {
		printf("html source file\n");

        ///NOTE: Reference may be freed elsewhere
		doc.reference = &content;
	} else {
		printf("Error: HTML pages must start <\n");
	}
	elix_html_parse(doc);
	return doc;
}

void elix_html_printNode(elix_html_node * node, size_t & d) {
	ASSERT(node);
	for (size_t var = 0; var < d; ++var) {
		printf("\t");
	}
	elix_html_node_object * obj = node->get();

	switch (obj->type) {
		case ELEMENT_RAWTEXT:
			if (obj->textContent.length)
				printf("> '%.*s'\n", obj->textContent.length, obj->textContent.string);
			break;
		case ELEMENT_VOID:
			if (obj->textContent.length)
				printf("'%.*s'\n", obj->textContent.length, obj->textContent.string);
			break;
		default:
			printf("[%s]\n", obj->name);
			break;
	}

	for (elix_html_node q: node->get()->children) {
		d++;
		elix_html_printNode(&q, d);
		d--;
	}
}
void elix_html_print(elix_html_document * doc) {
	size_t d = 0;

	if (doc->root.get()->type) {
		elix_html_printNode(&doc->root,d);
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

