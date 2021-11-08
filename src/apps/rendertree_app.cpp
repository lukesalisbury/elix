#include "elix_endian.hpp"
#include "elix_html.hpp"
#include "elix_fpscounter.hpp"
#include "elix_os_window.hpp"

#include "elix_rgbabuffer.hpp"

#include "elix_cstring.hpp"
#include "elix_program.hpp"
#include "elix_os.hpp"



static elix_fpscounter fps;
static elix_program_info program_info;

void update_buffer_randomly(elix_graphic_data * buffer) {
	uint32_t * p = buffer->pixels;
	uint32_t c = 0xFFFFFFFF;
	for ( uint32_t q = 0; q < buffer->pixel_count; q++, p++ ) {
		if ( q % (buffer->width*5) == 0 ) {
		#if RAND_MAX == 32767
			c = (uint32_t)(rand() * rand()) | 0xFF000000;
		#else
			c = rand() | 0xFF000000;
		#endif
		}
		*p = c;
	}
}

bool elix_cstring_equal(const char * A, const char * B) {
	bool result = (A == B);

	if(A && B)
	{
		while(*A && *B && (*A == *B))
		{
			++A;
			++B;
		}

		result = ((*A == 0) && (*B == 0));
	}

	return result ;
}


// Hot reloading code copy from https://github.com/RandyGaul/C-Hotloading
#include <Windows.h>
#include <cstdio>


typedef void (*LoopType)( elix::html::document & html, rbgabuffer_context * bitmap_context);
LoopType LoopPtr;
HMODULE GameDLL;
FILETIME GameDLLWriteTime;

FILETIME Win32GetLastWriteTime( char* path )
{
	FILETIME time = {};
	WIN32_FILE_ATTRIBUTE_DATA data;

	if ( GetFileAttributesEx( path, GetFileExInfoStandard, &data ) )
		time = data.ftLastWriteTime;

	return time;
}

void UnloadGameDLL( )
{
	FreeLibrary( GameDLL );
	GameDLL = 0;
	LoopPtr = 0;
}

void LoadGameDLL( )
{
	WIN32_FILE_ATTRIBUTE_DATA unused;
	if ( !GetFileAttributesEx( "lock.tmp", GetFileExInfoStandard, &unused ) )
	{
		UnloadGameDLL( );
		CopyFile( "rendertree_code.so", "rendertree_code.tso", 0 );
		GameDLL = LoadLibrary( "rendertree_code.tso" );

		if ( !GameDLL )
		{
			DWORD err = GetLastError( );
			printf( "Can't load lib: %ld\n", err );
			return;
		}

		LoopPtr = (LoopType)GetProcAddress( GameDLL, "Loop" );
		if ( !LoopPtr )
		{
			DWORD err = GetLastError( );
			printf( "Cant load func: %ld\n", err );
			return;
		}

		GameDLLWriteTime = Win32GetLastWriteTime( "rendertree_code.so" );
	}
}



void test_elix_html() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOG_MESSAGE("--- Elix HTML Parser -----------------------------------");

	std::string test_html(R"TEXT(<!DOCTYPE html><html>
	<!-- Commement --><body>Hello <![CDATA[ sdaghkl
 asd]] ]]> 🐨 World 🐱‍🚀<div><div></div></div></body></html>)TEXT");

	LOG_MESSAGE("%s", test_html.c_str());
	LOG_MESSAGE("--------------------------------------------------------");
	elix::html::document html = elix::html::open(test_html);
	elix::html::print(&html);

	elix_os_window * w = elix_os_window_create({{600, 500}}, {1,1});

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );


	__asm__ volatile("int $0x03"); // -exec file rendertree_code.tso
	while(elix_os_window_handle_events(w) ) {
		if ( w->flags & EOE_WIN_CLOSE ) {
			elix_os_window_destroy(w);
			break;
		}

		FILETIME newTime = Win32GetLastWriteTime( "rendertree_code.so" );

		if ( CompareFileTime( &newTime, &GameDLLWriteTime ) ) {
			LoadGameDLL( );
			//elix::html::clear_render_tree(&html);
		}
		if ( LoopPtr ) {
			LoopPtr( html, bitmap_context);
		}
		fps.update();
		elix_os_window_render(w);
		//elix_os_system_idle(16000);
	}

	elix_os_window_destroy( w );
	delete w;
}



void test_run( const char* name, void (*test)() ) {
	if ( test ) {
		struct timespec start, end;
		//clock_gettime(CLOCK_MONOTONIC, &start );
		test();
		//clock_gettime(CLOCK_MONOTONIC, &end );

		//double elapse = difftime(end.tv_sec, start.tv_sec) + ((double)(end.tv_nsec - start.tv_nsec)/1.0e9);
		//NAMEDLOG_MESSAGE(name, "--- Took: %f ------------------------------------------", elapse);
	}
}



int main(int UNUSEDARG argc, char UNUSEDARG * argv[])
{
	//SetConsoleOutputCP(65001);

	printf("Console Test: %s\n", "🎒🤔🐱‍🚀" );

	program_info = elix_program_info_create(argv[0], "Elix Test Program", "0.4", nullptr);
	LoadGameDLL( );

	test_elix_html();
	return 0;
}



