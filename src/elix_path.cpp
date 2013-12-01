/****************************
Copyright © 2006-2012 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/

#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstring>
#include <ctype.h>
#include "elix_path.h"
#include "elix_string.h"

#ifdef __GNUWIN32__
	#ifndef _WIN32_IE
		#define _WIN32_IE 0x0600
	#endif
	#include <io.h>
	#include <shlobj.h>
	#include <direct.h>
#endif
#ifdef ANDROID_NDK
#include <SDL.h>
#include <android/native_activity.h>
#endif

namespace elix {
	namespace path {

		std::string _user = "/ElixTemporaryContent/";
		std::string program_name = "ElixTemporaryContent";
		std::string document_name = "";
		std::string binary_dir = "/";
		std::string program_version = "";

		void printInfo();

		void StripUnwantedCharacters( std::string & str )
		{
			size_t pos;
			while( str.length() && str.at(0) == '.' ) // Don't start with a .
				str.erase( 0, 1 );
			while( (pos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-.")) != std::string::npos )
				str.erase( pos, 1 );
		}

		bool SetProgramName( std::string name, std::string version, std::string document, std::string executable )
		{
			/* Clear non alphanumeric characters */
			StripUnwantedCharacters(name);
			if ( name.length() )
			{
				program_name = name;
			}
			else
			{
				program_name = "ElixTemporaryContent";

			}

			program_version = version;

			/* Use Lower case program_name followed by version */
			_user.assign( 1, ELIX_DIR_SEPARATOR);
			_user.append( program_name );
			_user.append( 1, '-');
			_user.append( program_version );
			_user.append( 1, ELIX_DIR_SEPARATOR);
			std::transform(_user.begin(), _user.end(), _user.begin(), tolower);


			binary_dir = elix::path::GetBase( executable, false );

			/* Clear non alphanumeric characters */
			StripUnwantedCharacters(document);
			if ( document.length() )
			{
				document_name.assign( 1, ELIX_DIR_SEPARATOR);
				document_name.append( document );
				document_name.append( 1, ELIX_DIR_SEPARATOR);
			}
			return true;
		}

		std::string GetBase( std::string path, bool trailing )
		{
			int lastslash = path.find_last_of( ELIX_DIR_SEPARATOR, path.length() );
			if ( lastslash == -1 )
			{
				return "."ELIX_DIR_SSEPARATOR;
			}
			if ( trailing )
			{
				lastslash += 1;
			}
			if ( lastslash >= 2 )
			{
				return path.substr( 0, lastslash );
			}
			return path;
		}

		std::string GetName( std::string path )
		{
			std::string new_path = path;
			int lastslash = new_path.find_last_of( ELIX_DIR_SEPARATOR, new_path.length() );
			if ( lastslash )
			{
				new_path = new_path.substr( lastslash + 1 );
			}
			if ( ELIX_DIR_SEPARATOR != '/' )
			{
				lastslash = new_path.find_last_of( ELIX_DIR_SEPARATOR, new_path.length() );
				if ( lastslash )
				{
					new_path = new_path.substr( lastslash + 1 );
				}
			}
			return new_path;
		}

		/* Directory Functions */
		/***********************
		 * elix::path::Exist
		 @ std::string directory
		 - return true if it's a directory exist
		*/
		bool Exist( std::string path )
		{
			if ( path.length() > 1 )
			{
				if ( path.at( path.length()-1 ) == ELIX_DIR_SEPARATOR )
				{
					path.erase(path.length()-1);
				}
				struct stat directory;
				if ( !stat( path.c_str(), &directory ) )
				{
					return S_ISDIR(directory.st_mode);
				}
			}
			return false;
		}

		/***********************
		 * elix::path::Create
		 @ std::string directory
		 - return true if it's a directory exist
		*/
		bool Create( std::string path )
		{
			if ( !Exist(path) )
			{
			#ifdef __GNUWIN32__
				_mkdir( path.c_str() );
			#else
				mkdir( path.c_str(), 0744 );
			#endif
			}
			return true;
		}

		/***********************
		 * elix::path::Children
		 @ std::string path
		 @ std::string subpath
		 @ std::vector<std::string> & list
		 @ bool deep
		 @ bool storepath
		 @ bool storedirectories
		 - return true if it's a directory exist
		*/
		bool Children( std::string path, std::string sub_path, std::vector<std::string> & list, bool deep, bool storepath, bool storedirectories )
		{
			dirent * entry = NULL;
			std::string dir_path = path + ELIX_DIR_SSEPARATOR + sub_path;
			std::string file_path = path;
			DIR * dir = opendir( dir_path.c_str() );

			if ( !dir )
			{
				return false;
			}

			while ( (entry = readdir(dir)) != NULL )
			{
				if ( (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) )
				{
					file_path = dir_path + ELIX_DIR_SSEPARATOR + entry->d_name;
					if ( elix::path::Exist( file_path ) )
					{
						if ( deep )
						{
							std::string dir_store(entry->d_name);
							Children( path, dir_store, list, false, storepath, storedirectories );
						}
						if ( storedirectories )
						{
							std::string name_store;

							if ( storepath )
							{
								name_store = file_path + ELIX_DIR_SSEPARATOR;
							}
							else if ( sub_path.length() )
							{
								name_store = sub_path + ELIX_DIR_SSEPARATOR + entry->d_name + ELIX_DIR_SSEPARATOR;
							}
							else
							{
								name_store =  entry->d_name;
								name_store.append(ELIX_DIR_SSEPARATOR);
							}
							list.push_back( name_store );
						}
					}
					else
					{
						std::string name_store;
						if ( storepath )
						{
							name_store = file_path;
						}
						else if ( sub_path.length() )
						{
							name_store = sub_path + ELIX_DIR_SSEPARATOR + entry->d_name;
						}
						else
						{
							name_store = entry->d_name;
						}
						list.push_back( name_store );
					}
				}
			}
			closedir(dir);
			return true;
		}





		/* User's Directory Functions */
		std::string Program( )
		{
			return binary_dir;
		}
	#if defined(DREAMCAST) || defined(__NDS__) || defined(GEKKO) || defined(FLASCC)

		std::string User( std::string subname, bool roaming )
		{
			#if defined (DREAMCAST)
			std::string full_directory = "/sd/";
			#else
			std::string full_directory = "/";
			#endif
			StripUnwantedCharacters( subname );

			if ( subname.length() )
			{
				full_directory += subname;
				full_directory.append(1, ELIX_DIR_SEPARATOR);
			}

			return full_directory;
		}

		std::string Documents( bool shared )
		{
			#if defined (DREAMCAST)
				return "/";
			#else
				return User(document_name);
			#endif

		}

		std::string Cache(std::string filename )
		{
		#if defined (DREAMCAST)
			return "/sd/cache/" + filename;
		#else
			return "/cache/" + filename ;
		#endif
		}

		std::string Resources( std::string subdir )
		{
			#if !defined (DREAMCAST)
			return User(subdir);
			#endif
			std::string full_directory = "/cd/";

			StripUnwantedCharacters( subdir );
			if ( subdir.length() )
			{
				full_directory += subdir;
				full_directory.append(1, ELIX_DIR_SEPARATOR);
			}

			return full_directory;

		}

	#elif defined(ANDROID_NDK)

		std::string User( std::string subname, bool roaming )
		{
			ANativeActivity * activity = (ANativeActivity*)SDL_AndroidGetActivity();

			std::string full_directory;

			full_directory = SDL_AndroidGetExternalStoragePath();
			full_directory.append(1, ELIX_DIR_SEPARATOR);

			StripUnwantedCharacters( subname );

			if ( subname.length() )
			{
				full_directory += subname;
				full_directory.append(1, ELIX_DIR_SEPARATOR);
			}
			Create( full_directory );
			return full_directory;
		}

		std::string Documents( bool shared )
		{
			return User(document_name);
		}

		std::string Cache(std::string filename )
		{
			return User(".cache") + filename ;
		}

		std::string Resources( std::string subdir )
		{
			std::string full_directory = User("share");

			full_directory.append( _user );

			Create( full_directory );
			if ( Exist( full_directory ) )
			{
				StripUnwantedCharacters( subdir );
				if ( subdir.length() )
				{
					full_directory += subdir;
				}
			}
			return full_directory;


		}

	#else

		std::string User( std::string subdir, bool roaming )
		{
			std::string full_directory = "./";
			bool valid = true;

		#if defined(__WINDOWS__) || defined(__GNUWIN32__)
			char directory[MAX_PATH];
			valid = SHGetSpecialFolderPath(NULL, directory, (roaming ? CSIDL_APPDATA : CSIDL_LOCAL_APPDATA), 0);
			if ( valid )
				full_directory = directory;
		#else
			/* POSIX + XDG */
			char * home_path = getenv( "XDG_DATA_HOME" );
			/* If XDG_DATA_HOME not set use HOME instead  */
			if ( home_path == NULL )
			{
				home_path = getenv( "HOME" );
				if ( home_path )
				{
					full_directory.assign(home_path);
					full_directory.append("/.local/share");
				}
				else
				{
					/* Fallback to current directory */
					full_directory.assign("./");
				}
			}
			else
			{
				full_directory.assign(home_path);
			}

		#endif
			/* Make sure the directory exist */
			full_directory.append(_user);
			Create( full_directory );

			if ( valid )
			{
				StripUnwantedCharacters( subdir );
				if ( subdir.length() )
				{
					full_directory += subdir;
					full_directory.append(1, ELIX_DIR_SEPARATOR);
				}
				Create( full_directory );
			}

			return full_directory;
		}

		std::string Documents( bool shared )
		{
			/* If not set with SetProgramName, return user directory */
			if ( !document_name.length() )
			{
				return User("documents");
			}

			std::string full_directory = "./";
			bool valid = true;

		#ifdef __GNUWIN32__
			char directory[MAX_PATH];
			valid = SHGetSpecialFolderPath(NULL, directory, (shared ? CSIDL_COMMON_DOCUMENTS : CSIDL_PROFILE), 0);
			if ( valid )
				full_directory.assign(directory);
			full_directory.append(document_name);
		#else
			if ( shared )
			{
				char * home_path = getenv( "XDG_DATA_DIRS" );
				if ( !home_path )
				{
					full_directory.assign("/usr/share");
					full_directory.append(document_name);
				}
				else
				{
					std::string path = home_path;
					std::vector<std::string> results;
					elix::string::Split( home_path, ":", &results );

					valid = false;

					if ( results.size()  )
					{
						for( uint32_t i = 0; i < results.size(); i++ )
						{
							path = results.at(i);
							path.append(document_name);
							if ( Exist(path) )
							{
								full_directory.assign(path);
								valid = true;
							}
						}
					}
				}

			}
			else
			{
				/* POSIX + XDG */
				char * home_path = getenv( "XDG_DATA_HOME" );
				/* If XDG_DATA_HOME not set use HOME instead  */
				if ( home_path == NULL )
				{
					home_path = getenv( "HOME" );
					if ( home_path )
					{
						full_directory.assign(home_path);
					}
					else
					{
						/* Fallback to current directory */
						full_directory.assign("./");
					}
				}
				else
				{
					full_directory.assign(home_path);
				}
				full_directory.append(document_name);
			}
		#endif


			if ( valid )
				return full_directory;
			else
				return "/";
		}

		std::string Cache(std::string filename )
		{
			std::string full_directory;
		#if defined (__GNUWIN32__)
			full_directory = elix::path::User( "cache", true );
		#else
			char * home_path = NULL;
			home_path = getenv( "XDG_CACHE_HOME" );
			if ( !home_path )
			{
				home_path = getenv( "HOME" );
				if (home_path)
				{
					full_directory.assign(home_path);
					full_directory.append("/.ElixCache/");
					mkdir(full_directory.c_str(), 0744);
				}
				else
				{
					full_directory.assign("./.ElixCache/");
					mkdir(full_directory.c_str(), 0744);
				}
			}
			else
			{
				full_directory.assign(home_path);
			}
			full_directory.append(_user);
		#endif

			Create( full_directory );

			StripUnwantedCharacters( filename );
			if ( filename.length() )
			{
				full_directory += filename;
			}
			return full_directory;
		}

		std::string Resources( std::string subdir )
		{
			std::string full_directory = "./";

			if ( Exist( binary_dir + ELIX_DIR_SSEPARATOR"share"ELIX_DIR_SSEPARATOR ) )
			{
				full_directory = binary_dir + ELIX_DIR_SSEPARATOR"share"ELIX_DIR_SSEPARATOR;
			}
			else
			{
				/* Takes into account the the binaries are stored in bin subdirectory */
				size_t pos = binary_dir.find_last_of(ELIX_DIR_SEPARATOR);
				if ( pos > 1 )
					full_directory = binary_dir.substr(0, pos);
				else
					full_directory = "..";
				full_directory.append( 1, ELIX_DIR_SEPARATOR);
				full_directory.append( "share" );
			}
			full_directory.append( _user );

			Create( full_directory );
			if ( Exist( full_directory ) )
			{
				StripUnwantedCharacters( subdir );
				if ( subdir.length() )
				{
					full_directory += subdir;
				}
			}
			return full_directory;
		}
	#endif

		void printInfo()
		{
			std::cout << "program_name: " << program_name << std::endl;
			std::cout << "program_version: " << program_version << std::endl;
			std::cout << "binary_dir: " << binary_dir << std::endl;
			std::cout << "User: " << User("") << std::endl;
			std::cout << "User Documents: " << Documents(false) << std::endl;
			std::cout << "Global Documents: " << Documents(true) << std::endl;
			std::cout << "Cache: " << Cache("") << std::endl;
			std::cout << "Resources: " << Resources("") << std::endl;
		}

	}
}

