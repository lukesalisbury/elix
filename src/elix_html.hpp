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
#include "elix_rendertree.hpp"

#include <string>
#include <vector>
#include <memory>
typedef std::basic_string<uint8_t> elix_sstring;

namespace elix {
	struct string_pointer {
		std::string * reference;
		bool ownReference = false;
		size_t offset;
		size_t length;
	};
	struct character {
		uint32_t value;
		uint8_t bytes;
		uint8_t padding;
		uint16_t codepage;
	};

	character getNextCharacter( std::string::iterator & text );

	namespace html {
		struct node_object;
		typedef std::shared_ptr<node_object> node;
		struct attr {
			std::string name;
			std::string value;
		};
		struct node_object {
			uint8_t type = 0;
			char name[16];
			string_pointer source;
			node parent;
			std::vector<node> children;
			std::vector<attr> attribute;
			std::string textContent;
			elix_rendertree_item render_item;
		};

		struct document {
			std::vector<node> nodes;
			std::string reference;
			elix_rendertree rendertree;
			node root;
		};

		struct status {
			size_t offset = 0;
			size_t length = 0;
		};

		elix::html::status parse(elix::html::document & doc, elix::html::status * lastStatus = nullptr);
		elix::html::document open(std::string content);
		void print(document * doc);
		void close(elix::html::document & doc);

		elix_rendertree build_render_tree(document * doc, elix_uv32_2 dimension);
		void clear_render_tree(document * doc);

	}
}
#endif // ELIX_HTML_HPP

