/****************************
Copyright © 2006-2014 Luke Salisbury
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
#include "elix_path.hpp"
#include "elix_string.hpp"
#include "elix_program.hpp"

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

	/* User's Directory Functions */
	#if defined(DREAMCAST) || defined(__NDS__) || defined(GEKKO) || defined(FLASCC)
	namespace directory {
		std::string User( std::string subname, bool roaming )
		{
			#if defined (DREAMCAST)
			std::string full_directory = "/sd/";
			#else
			std::string full_directory = "/";
			#endif
			elix::string::StripUnwantedCharacters( subname );

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

			elix::string::StripUnwantedCharacters( subdir );
			if ( subdir.length() )
			{
				full_directory += subdir;
				full_directory.append(1, ELIX_DIR_SEPARATOR);
			}

			return full_directory;

		}
	}
	#elif defined(ANDROID_NDK)

	// Android
	namespace directory {
		std::string User( std::string subname, bool roaming )
		{
			ANativeActivity * activity = (ANativeActivity*)SDL_AndroidGetActivity();

			std::string full_directory;

			full_directory = SDL_AndroidGetExternalStoragePath();
			full_directory.append(1, ELIX_DIR_SEPARATOR);

			elix::string::StripUnwantedCharacters( subname );

			if ( subname.length() )
			{
				full_directory += subname;
				full_directory.append(1, ELIX_DIR_SEPARATOR);
			}
			elix::path::Create( full_directory );
			return full_directory;
		}

		std::string Documents( bool shared )
		{
			return User(elix::program::document_name);
		}

		std::string Cache(  )
		{
			elix::path::Create( User(".cache") );
			return User(".cache");
		}

		std::string Cache( std::string filename )
		{
			elix::string::StripUnwantedCharacters( filename );
			return Cache() + filename ;
		}

		std::string Resources( std::string subdir )
		{
			std::string full_directory = User("share");

			full_directory.append( elix::program::_user );

			elix::path::Create( full_directory );
			if ( elix::path::Exist( full_directory ) )
			{
				elix::string::StripUnwantedCharacters( subdir );
				if ( subdir.length() )
				{
					full_directory += subdir;
				}
			}
			return full_directory;


		}
	}
	#else
	// PC
	namespace directory {
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
			full_directory.append(elix::program::_user);
			elix::path::Create( full_directory );

			if ( valid )
			{
				elix::string::StripUnwantedCharacters( subdir );
				if ( subdir.length() )
				{
					full_directory += subdir;
					full_directory.append(1, ELIX_DIR_SEPARATOR);
				}
				elix::path::Create( full_directory );
			}

			return full_directory;
		}

		std::string Documents( bool shared )
		{
			/* If not set with SetProgramName, return user directory */
			if ( !elix::program::document_name.length() )
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
			full_directory.append(elix::program::document_name);
		#else
			if ( shared )
			{
				char * home_path = getenv( "XDG_DATA_DIRS" );
				if ( !home_path )
				{
					full_directory.assign("/usr/share");
					full_directory.append(elix::program::document_name);
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
							path.append(elix::program::document_name);
							if ( elix::path::Exist(path) )
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
				full_directory.append(elix::program::document_name);
			}
		#endif


			if ( valid )
				return full_directory;
			else
				return "/";
		}

		std::string Cache( )
		{
			std::string full_directory;
		#if defined (__GNUWIN32__)
			full_directory = elix::directory::User( "cache", true );
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
			full_directory.append(elix::program::_user);
		#endif

			elix::path::Create( full_directory );

			return full_directory;
		}


		std::string Cache( std::string filename )
		{
			std::string full_directory = Cache();

			elix::string::StripUnwantedCharacters( filename );
			if ( filename.length() )
			{
				full_directory += filename;
			}
			else
			{
				full_directory += "temp";
			}
			return full_directory;
		}

		std::string Resources( std::string subdir )
		{
			std::string full_directory = "./";
			std::string share_directory = elix::program::RootDirectory() + ELIX_DIR_SSEPARATOR"share"ELIX_DIR_SSEPARATOR;


			if ( elix::path::Exist( share_directory ) )
			{
				full_directory = share_directory;
			}
			else
			{
				share_directory = elix::program::RootDirectory();
				/* Takes into account the the binaries are stored in bin subdirectory */
				size_t pos = share_directory.find_last_of(ELIX_DIR_SEPARATOR);
				if ( pos > 1 )
					full_directory = share_directory.substr(0, pos);
				else
					full_directory = "..";
				full_directory.append( 1, ELIX_DIR_SEPARATOR);
				full_directory.append( "share" );
			}
			full_directory.append( elix::program::_user );

			elix::path::Create( full_directory );
			if ( elix::path::Exist( full_directory ) )
			{
				elix::string::StripUnwantedCharacters( subdir );
				if ( subdir.length() )
				{
					full_directory += subdir;
				}
			}
			return full_directory;
		}
	}
	#endif

}

