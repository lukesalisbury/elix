#ifndef _ELIX_PATH_H_
#define _ELIX_PATH_H_

#ifdef __GNUWIN32__
	#define ELIX_DIR_SEPARATOR '\\'
	#define ELIX_DIR_SSEPARATOR "\\"
#else
	#define ELIX_DIR_SEPARATOR '/'
	#define ELIX_DIR_SSEPARATOR "/"
#endif
#include <vector>
#include <string>

namespace elix {
	namespace path {
		bool SetProgramName( std::string name, std::string version, std::string document, std::string executable );
		std::string GetBase( std::string path, bool trailing );
		std::string GetName( std::string path );

		void StripUnwantedCharacters( std::string & str );

		/* Directory Functions */
		bool Exist(std::string path );
		bool Create( std::string path );
		bool Children(std::string path, std::string sub_path, std::vector<std::string> & list, bool deep, bool storepath, bool storedirectories );


		/* Returns Private or Public Document Folder */
		std::string Documents( bool shared );

		/* Returns User Data directory with subfolder. XDG_DATA_HOME/ProgramName-ProgramVersion/subdir or CSIDL_Local_APPDATA/ProgramName-ProgramVersion/subdir */
		std::string User(std::string subdir, bool roaming = false );

		/* Returns User Cache directory with file . XDG_CACHE_HOME/ProgramName/filename or CSIDL_APPDATA/ProgramName/cache/filename */
		std::string Cache(std::string filename );

		/* Returns Program Resources directory with subfolder. binary_dir/../share/ProgramName-ProgramVersion/subdir */
		std::string Resources(std::string subdir );

		/* Returns Binary Directory */
		std::string Program( );
	}
}

#endif
