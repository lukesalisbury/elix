#ifndef _ELIX_FILE_H_
#define _ELIX_FILE_H_

#include <cstdio>
#include <fstream>
#include "elix_path.hpp"
#include "elix_intdef.h"

typedef FILE* file_pointer;
typedef void* data_pointer;

namespace elix {
	class File
	{
		public:
			File( std::string path, bool write = false);
			~File();
		private:
			file_pointer handle;
			std::fstream write_log_handle;
			std::fstream read_log_handle;
			bool writable;
			std::string error;
			std::string path;
			uint32_t _length;

		public:
			void (*ErrorCallback)(std::string);
			bool Exist();
			uint32_t Length();
			int32_t Seek( int32_t pos );
			int32_t Tell();
			int32_t Scan( int32_t startPostion, uint8_t * needle, uint32_t needleLength );
			bool EndOfFile();
			bool Read( data_pointer buffer, uint32_t size, uint32_t count );
			uint32_t Read2( data_pointer buffer, uint32_t size, uint32_t count );
			uint32_t ReadAll( data_pointer * buffer, bool addnull = false );
			uint32_t Read_uint32( bool sysvalue = false );
			uint16_t Read_uint16( bool sysvalue = false );
			uint8_t Read_uint8( );
			std::string ReadString();
			bool Read(  std::string * str, uint32_t length = 0 );
			bool ReadLine( std::string * str );
			bool Write( data_pointer buffer, uint32_t size, uint32_t count );
			bool Write( uint32_t value, bool sysvalue = false );
			bool Write( uint16_t value, bool sysvalue = false );
			bool Write( uint8_t value );
			bool Write( std::string value );
			bool WriteString( std::string value );



			/* Function to use with testing something */
			bool WriteWithLabel( std::string label, data_pointer buffer, uint32_t size, uint32_t count );
			bool WriteWithLabel( std::string label, uint32_t value, bool sysvalue = false );
			bool WriteWithLabel( std::string label, uint16_t value, bool sysvalue = false );
			bool WriteWithLabel( std::string label, uint8_t value );
			bool WriteWithLabel( std::string label, std::string value );


			bool ReadWithLabel( std::string label, data_pointer buffer, uint32_t size, uint32_t count );
			uint32_t Read2WithLabel( std::string label, data_pointer buffer, uint32_t size, uint32_t count );
			uint32_t Read_uint32WithLabel( std::string label, bool sysvalue = false );
			uint16_t Read_uint16WithLabel( std::string label, bool sysvalue = false );
			uint8_t Read_uint8WithLabel( std::string label );
			std::string ReadStringWithLabel( std::string label );
			bool ReadWithLabel( std::string label, std::string * str, uint32_t length = 0 );



	};
}
#endif
