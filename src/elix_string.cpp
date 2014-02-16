/****************************
Copyright © 2006-2012 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstring>
#include "elix_string.hpp"

namespace elix {
	namespace string {
		void StripUnwantedCharacters( std::string & str )
		{
			size_t pos;
			while( str.length() && str.at(0) == '.' ) // Don't start with a .
				str.erase( 0, 1 );
			while( (pos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-.")) != std::string::npos )
				str.erase( pos, 1 );
		}

		void Replace( std::string & str, std::string find, std::string replace )
		{
			std::string::size_type look = 0;
			std::string::size_type found;

			while((found = str.find(find, look)) != std::string::npos)
			{
				str.replace(found, find.size(), replace);
				look = found + replace.size();
			}
		}
		void Split( std::string str, std::string delim, std::vector<std::string> * results )
		{
			if ( str.size() && delim.size() )
			{
				std::string::size_type lastPos = str.find_first_not_of(delim, 0);
				std::string::size_type pos = str.find_first_of(delim, lastPos);

				while ( std::string::npos != pos || std::string::npos != lastPos )
				{
					results->push_back( str.substr(lastPos, pos - lastPos) );
					lastPos = str.find_first_not_of(delim, pos);
					pos = str.find_first_of(delim, lastPos);
				}
			}
		}
		void Trim( std::string * source )
		{
			if ( source->size() )
			{
				source->erase(source->find_last_not_of("\r\n")+1);
				source->erase(0, source->find_first_not_of("\r\n"));
			}
		}

		void Truncate( std::string & source, size_t requested_length )
		{
			if ( source.length() > requested_length )
			{
				size_t center_char = requested_length/2;
				size_t remove_char_count = source.length() - requested_length;

				//center_char -= !!(remove_char_count%2);

				source.replace( center_char, remove_char_count, "..." ); // …
			}
		}

		uint32_t Hash( std::string str )
		{
			uint32_t hash = 0;
			size_t i;

			for (i = 0; i < str.length(); i++)
			{
				hash += str[i];
				hash += (hash << 10);
				hash ^= (hash >> 6);
			}
			hash += (hash << 3);
			hash ^= (hash >> 11);
			hash += (hash << 15);
			return hash;
		}
		std::string FromInt(int32_t value)
		{
			std::string str;
			std::ostringstream stream("");
			stream << value;
			str = stream.str();
			return str;
		}
		int32_t ToInt32(std::string string)
		{
			int32_t value;
			std::stringstream stream(string);
			stream >> value;
			return value;
		}
		uint16_t ToIntU16(std::string string)
		{
			uint16_t value;
			std::stringstream stream(string);
			stream >> value;
			return value;
		}
		uint8_t ToIntU8(std::string string)
		{
			uint16_t value;
			std::stringstream stream(string);
			stream >> value;
			return (uint8_t)value;
		}

		bool HasPrefix( const std::string source, const std::string prefix )
		{
			std::string::size_type look = 0;
			std::string::size_type found;

			if ((found = source.find(prefix, 0)) != std::string::npos)
			{
				if ( found == 0)
					return true;
			}
			return false;
		}

		bool HasSuffix( const std::string source, const std::string suffix )
		{
			std::string::size_type look = 0;
			std::string::size_type found;
			std::string::size_type offset = source.length() - suffix.length();

			if ((found = source.rfind(suffix, std::string::npos)) != std::string::npos)
			{
				if ( found == offset )
					return true;
			}
			return false;
		}
	}
}

