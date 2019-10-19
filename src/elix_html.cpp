#include "elix_html.hpp"
#include <codecvt>
#include <locale>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

namespace elix {

	character getNextCharacter( std::string::iterator & text ) {
		character char32 = {0,1,0,1};

		uint8_t char8 = 0;
		uint32_t next;

		char8 = (unsigned)*text;
		if ( char8 < 128 ) {
			char32.value = char8;
			char32.codepage = 0; //ASCII
		} else if ( char8 > 193 && char8 < 224 ) {
			text++;
			next = (unsigned)(*text);

			char32.value = ((char8 << 6) & 0x7ff) + (next & 0x3f);
		} else if ( char8 > 223 && char8 < 240 ) {
			text++;
			next = (*text) & 0xff;
			char32.value = ((char8 << 12) & 0xffff) + ((next << 6) & 0xfff);

			text++;
			next = (*text) & 0x3f;
			char32.value += next;
		} else if ( char8 > 239 && char8 < 245 ) {
			text++;
			next = (*text) & 0xff;
			char32.value = ((char8 << 18) & 0xffff) + ((next << 12) & 0x3ffff);

			text++;
			next = (*text) & 0xff;
			char32.value += (next << 6) & 0xfff;

			text++;
			next = (*text) & 0x3f;
			char32.value += next;

		} else {
			char32.value = 0xFFFD;
		}
		text++;
		return char32;
	}

	/*
	root
	- Element
	- @attribute
	- @children
	*/

	namespace html {
		enum element_type {
			ELEMENT_FOREIGN,
			ELEMENT_VOID,
			ELEMENT_TEMPLATE,
			ELEMENT_RAWTEXT,
			ELEMENT_RAWTEXTAREA,
			ELEMENT_NORMAL
		};
		enum parse_state { PARSE_NONE, PARSE_TAG,PARSE_START_TAG,PARSE_END_TAG, PARSE_ATTRIBUTE, PARSE_COMMENT, PARSE_SCAN_DOCTYPE, PARSE_SCAN_CDATA, PARSE_SCAN_COMMENT, PARSE_SCAN_TEXT, PARSE_ERROR = 0xFF};

		struct string_match {
			uint32_t string[7];
			union {
				uint8_t size;
				uint32_t size32;
			};
			parse_state result;
		};

		struct parse_next_info {
			parse_state state = PARSE_NONE;
			uint8_t index = 0;
			uint8_t padding[3] = {0};
		};


		static string_match doctype_start = { {'d','o','c','t','y','p','e'}, {7}, PARSE_SCAN_DOCTYPE };
		static string_match comment_start = { {'-','-'}, {2}, PARSE_SCAN_COMMENT };
		static string_match comment_end = { {'-','-', '>'}, {3}, PARSE_NONE };
		static string_match cdata_start = { {'[','C','D','A','T','A','['}, {7}, PARSE_SCAN_CDATA };
		static string_match cdata_end = { {']',']','>'}, {3}, PARSE_NONE };

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

		inline bool doesStringMatch(uint32_t c, parse_next_info & info, string_match & needle, parse_state & results) {
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

		inline bool doesInsensitiveStringMatch(uint32_t c, parse_next_info & info, string_match & needle, parse_state & results) {
			return doesStringMatch(lowerCaseASCII(c), info, needle, results);
		}

		inline elix::html::node pushNode(elix::html::node current_node, document & doc) {
			elix::html::node next_node;
			if ( current_node == nullptr ) {
				next_node = doc.root;
			} else {
				next_node = std::make_shared<node_object>();
				doc.nodes.push_back(next_node);
				next_node.get()->parent = current_node;
				current_node.get()->children.push_back(next_node);
			}
			return next_node;
		}

		inline elix::html::node pushElementNode(elix::html::node current_node, document & doc, std::u32string & buffer) {
			std::wstring_convert<std::codecvt<char32_t, char, mbstate_t>, char32_t> string_convert;
			elix::html::node next_node = pushNode(current_node,doc);
			std::string name = string_convert.to_bytes(buffer);
			memcpy(next_node.get()->name, name.c_str(), 16);
			next_node.get()->type = ELEMENT_NORMAL;
			buffer.clear();
			return next_node;
		}

		inline elix::html::node pushVoidNode(elix::html::node current_node, document & doc, std::u32string & buffer) {
			std::wstring_convert<std::codecvt<char32_t, char, mbstate_t>, char32_t> string_convert;
			elix::html::node next_node = pushNode(current_node,doc);
			next_node.get()->textContent = string_convert.to_bytes(buffer);
			next_node.get()->type = ELEMENT_VOID;
			buffer.clear();
			return next_node;
		}
		inline elix::html::node popNode(elix::html::node current_node) {
			return current_node.get()->parent;
		}

		elix::html::status parse(elix::html::document & doc, elix::html::status * lastStatus) {
			std::wstring_convert<std::codecvt<char32_t, char, mbstate_t>, char32_t> string_convert;
			elix::html::node current_node;
			elix::html::status current;

			current.length = doc.reference.length();
			if ( lastStatus != nullptr ) {
				//TODO: Continue parsing from a point
				if ( lastStatus->offset > current.length ) {
					//Pasted the end of file
				}
				if ( lastStatus->length > current.length ) {
					//File Changed
				}
			}

			if ( current.length < 5 ) {
				return current;
			}

			parse_state state = PARSE_NONE;
			std::u32string buffer;
			std::string prefix = "";

			character char32;
			std::string::iterator text = doc.reference.begin();
			if ( current.offset ) {
				text += (signed)current.offset;
			}
			string_convert.to_bytes(buffer);

			size_t d = 0;
			parse_next_info reset_parse_next_info;
			parse_next_info comment_next;
			parse_next_info scan_next;

			do {
				char32 = getNextCharacter(text);
				if ( char32.value ) {
					switch (state) {
						case PARSE_TAG: {
							if ( isVoidTagChar(char32.value) ) {
								state = STATECHANGE(PARSE_END_TAG, d);
							} else if ( isValidNameChar(char32.value, !!buffer.length()) ) {
								state = STATECHANGE(PARSE_START_TAG, d);
								buffer.append(1, char32.value);
							} else if ( char32.value == '!') {
								state = STATECHANGE(PARSE_COMMENT, d);
								comment_next = reset_parse_next_info;
							} else {
								state = STATECHANGE(PARSE_ERROR, d);
								printf("Error Parsing Tag at char %zu\n", current.offset);
								return current;
							}
							break;
						}
						case PARSE_START_TAG: {
							if ( isValidNameChar(char32.value, 0) ) {
								state = STATECHANGE(PARSE_START_TAG, d);
								buffer.append(1, char32.value);
							} else if ( char32.value == ' ' ) {
								current_node = pushElementNode(current_node, doc,buffer);
								state = STATECHANGE(PARSE_ATTRIBUTE, d);
							} else if ( char32.value == '>' ) {
								current_node = pushElementNode(current_node, doc,buffer);
								state = STATECHANGE(PARSE_NONE, d);
								d++;
							} else {

							}
							break;
						}
						case PARSE_SCAN_DOCTYPE: {
							//TODO check if > is not in quotes
							if ( char32.value == '>' ) {
								printf("Doctype '%s'\n", string_convert.to_bytes(buffer).c_str());
								state = STATECHANGE(PARSE_NONE, d);
								buffer.clear();
							} else {
								buffer.append(1, char32.value);
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
									current_node = pushVoidNode(current_node, doc, buffer);
									current_node = popNode(current_node);
									state = STATECHANGE(PARSE_NONE, d);
								}
							} else {
								if ( scan_next.index ) {
									//else if comment_end is semi found and scan_index > 0, append to buffer. parse stays the same
									for (uint8_t i = 0; i < scan_next.index && i < comment_end.size; i++) {
										buffer.append(1, comment_end.string[i]);
									}
									scan_next.index = 0;
								}
								//else add to buffer
								buffer.append(1, char32.value);
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
									current_node = pushVoidNode(current_node, doc, buffer);
									current_node = popNode(current_node);
									state = STATECHANGE(PARSE_NONE, d);
								}
							} else {
								if ( scan_next.index ) {
									//else if comment_end is semi found and scan_index > 0, append to buffer. parse stays the same
									for (uint8_t i = 0; i < scan_next.index && i < cdata_end.size; i++) {
										buffer.append(1, cdata_end.string[i]);
									}
									scan_next.index = 0;
								}

								//else add to buffer
								buffer.append(1, char32.value);
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
									printf("Error Parsing <! Tag at char %zu\n%s\n%*c^\n", current.offset, doc.reference.substr( current.offset < 4 ? 0 :current.offset - 4, 8).c_str(), current.offset < 4 ? current.offset-1 : 3, ' ' );
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
								printf("Error Parsing <! Tag at char %zu\n%s\n%*c^\n", current.offset, doc.reference.substr( current.offset < 4 ? 0 :current.offset - 4, 8).c_str(), current.offset < 4 ? current.offset-1 : 3, '-' );
								return current;
							}
							comment_next.index++;
							break;
						}
						case PARSE_END_TAG: {
							if ( isValidNameChar(char32.value, 0) ) {
								state = STATECHANGE(PARSE_END_TAG, d);
								buffer.append(1, char32.value);
							} else if ( char32.value == '>' ) {
								state = STATECHANGE(PARSE_NONE, d);
								current_node = popNode(current_node);
								buffer.clear();
								d--;
							}
							break;
						}
						default: {
							if ( isOpenTagChar(char32.value) ) {
								if ( state == PARSE_SCAN_TEXT) {
									current_node = pushNode(current_node, doc);
									memcpy(current_node.get()->name, "TextNode", 9);
									current_node.get()->type = ELEMENT_RAWTEXT;
									current_node.get()->textContent = string_convert.to_bytes(buffer);
									current_node = popNode(current_node);
								}
								state = STATECHANGE(PARSE_TAG, d);
								buffer.clear();
							} else {
								state = STATECHANGE(PARSE_SCAN_TEXT, d);
								buffer.append(1, char32.value);
							}
						}
					}
				}
				current.offset++;
			} while (char32.value && current.offset < current.length );

			return current;
		}

		elix::html::document open(std::string content) {
			elix::html::document doc;
			doc.root = std::make_shared<node_object>();
			if ( content.at(0) == '<'){
				printf("html source file\n");
				doc.reference = content;

			} else {
				printf("TODO: Open HTML file\n");
			}
			elix::html::parse(doc);
			return doc;
		}

		void printNode(elix::html::node * node, size_t & d) {
			ASSERT(node);
			for (size_t var = 0; var < d; ++var) {
				printf("\t");
			}
			node_object * obj = node->get();

			switch (obj->type) {
				case ELEMENT_RAWTEXT:
					printf("> '%s'\n", obj->textContent.c_str());
					break;
				case ELEMENT_VOID:
					printf("'%s'\n", obj->textContent.c_str());
					break;
				default:
					printf("> %s\n", obj->name);
					break;
			}

			for (elix::html::node q: node->get()->children) {
				d++;
				printNode(&q, d);
				d--;
			}
		}
		void print(elix::html::document * doc) {
			size_t d = 0;

			if (doc->root.get()->type) {
				printNode(&doc->root,d);
			} else {
				printf("No Root Node found\n");
			}
		}
		elix_rendertree get_render_tree(document * doc) {

			if (doc->root.get()->type) {

			} else {

			}

			return doc->rendertree;
		}

	}
}
