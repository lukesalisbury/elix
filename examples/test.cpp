#include "elix_endian.h"
#include "elix_html.h"
#include "elix_html_rendertree.h"
#include "extra/elix_fpscounter.hpp"
#include "window/elix_os_window.hpp"

#include "elix_rgbabuffer.h"

#include "elix_cstring.h"
#include "elix_os.h"
#include "elix_file.h"
#include "elix_hashmap.h"

#define LOG_INDENT(M, ...) printf("\t" M "\n", ##__VA_ARGS__)
#define LOG_TEXT(M, ...) printf(M "\n", ##__VA_ARGS__)
#define LOG_HR() printf("--------------------------------------------------------" "\n")


static elix_fpscounter fps;
static elix_program_info program_info;
static elix_consent program_consent = {true, true, nullptr};

function_results print_rgbabuffer(rbgabuffer_context * ctx) {
	printf("   0%*c%d\n", ctx->memory->width-2, ' ', ctx->memory->width  );
	uint32_t colour;
	for (uint32_t y = 0; y < ctx->memory->height; ++y) {
		printf("%02d:", y);
		for (uint32_t x = 0; x < ctx->memory->width; ++x) {
			//TODO Redo this
			colour = rbgabuffer_get_pixel(ctx, x, y);
			switch (colour) {
				case 0xDEADC0DE:
					printf("~");
				break;
				case 0xFFEEEEEE:
					printf("#");
				break;
				case 0xFFAAAAAA:
					printf("$");
				break;
			case 0xFFFFf000:
				printf("*");
			break;
			case 0xFFFF00FF:
				printf("+");
			break;
				default:
					printf("_");
				break;
			}
		}
		printf("\n");
	}
	return RESULTS_UNKNOWN;
}	

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
	if (A && B) {
		while(*A && *B && (*A == *B)) {
			++A;
			++B;
		}
		result = ((*A == 0) && (*B == 0));
	}
	return result;
}


function_results test_elix_endian() {
	uint32_t ul = 0x12030456;
	uint32_t uln = elix_endian_network32(ul);
	uint32_t ulh = elix_endian_host32(uln);
	LOG_INDENT("elix::endian:native 0x%08x", ul);
	LOG_INDENT("elix::endian:net32  0x%08x", uln);
	LOG_INDENT("elix::endian:host32 0x%08x", ulh);


	uint16_t us = 0x1203;
	uint16_t usn = elix_endian_network16(us);
	uint16_t ush = elix_endian_host16(usn);
	LOG_INDENT("elix::endian:native 0x%04x", us);
	LOG_INDENT("elix::endian:net16  0x%04x", usn);
	LOG_INDENT("elix::endian:host16 0x%04x", ush);
	return RESULTS_UNKNOWN;
}


/*
function_results test_elix_rendertree() {

	LOG_INDENT("--- Elix Rendertree ------------------------------------");

	elix_os_window * w = elix_os_window_create({{600, 500}}, {1,1});

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );
	elix_rendertree tree;

	elix_rendertree_to_rgbabuffer(&tree, bitmap_context, 1);
	while(elix_os_window_handle_events(w) ) {
		if ( w->flags & EOE_WIN_CLOSE ) {
			elix_os_window_destroy(w);
			break;
		}

		fps.update();
		elix_os_window_render(w);
		//elix_os_system_idle(16000);
	}

	elix_os_window_destroy( w );
	delete w;
	return RESULTS_UNKNOWN;
}
*/
elix_string_buffer elix_string_buffer_new(const char * string, size_t length) {
    elix_string_buffer str;
	if ( length >= __UINT16_MAX__ || string == nullptr || length == 0) {
		///TODO: Handle long text
		return str;
	}
    str.length = elix_cstring_length_stupid_c(string);
    str.allocated = length + 1;
    str.data = (uint8_t*)calloc(1, str.allocated);
	memcpy(str.data, string, str.length);
	str.iter = str.data;
	str.location = 0;
    return str;
}

const char * test_elix_html_string = R"TEXT(<!DOCTYPE html><html>
	<!-- Commement --><body>Hello <![CDATA[ sdaghkl
 asd]] ]]> 🐨 World 🐱‍🚀<div class="asdd" fail="this"><div tag>test</div>🐨🐨</div></body></html>)TEXT";

function_results test_elix_html() {
	elix_string_buffer test_html = elix_string_buffer_new(test_elix_html_string, 512);
	LOG_INDENT("Source", "");
	LOG_INDENT("%*s", test_html.length, test_html.data);

	size_t before,  after;
	before = elix_os_memory_usage();

	elix_html_document * html = elix_html_open(&test_html);

	LOG_INDENT("Parsed", "");
	elix_html_print(html);

	elix_html_close(html);

	after = elix_os_memory_usage();

	NULLIFY(html);

	LOG_INDENT("Memory Usage: Before: " pZU ", After: " pZU ", Diff: " pZU, before, after, after - before);

	return RESULTS_UNKNOWN;
}


function_results test_elix_html_window() {
	elix_string_buffer test_html = elix_string_buffer_new(test_elix_html_string, 512);

	elix_html_document * html = elix_html_open(&test_html);


	elix_os_window * w = elix_os_window_create({{600, 500}}, {1,1});

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );
	elix_rendertree tree = elix_html_build_rendertree(html, bitmap_context->dimensions);
	elix_rendertree_to_rgbabuffer(&tree, bitmap_context, 1);

	while(elix_os_window_handle_events(w) ) {
		if ( w->flags & EOE_WIN_CLOSE ) {
			elix_os_window_destroy(w);
			break;
		}

		fps.update();
		elix_os_window_render(w);
		//elix_os_system_idle(16000);
	}

	elix_os_window_destroy( w );
	delete w;
	
	elix_html_close(html);

	return RESULTS_UNKNOWN;
}

function_results test_elix_os_window() {
	elix_os_window * w = elix_os_window_create({{600, 400}}, {4,4});

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );

	fps.start();

	rbgabuffer_BeginPath(bitmap_context);
	rbgabuffer_Rect(bitmap_context, 80, 80, 120,30);
	rbgabuffer_FillColor(bitmap_context, 0xFFFF00FF);
	rbgabuffer_Fill(bitmap_context);

	rbgabuffer_BeginPath(bitmap_context);
	rbgabuffer_MoveTo(bitmap_context, 20.0, 0.0);
	rbgabuffer_LineTo(bitmap_context, 40.0, 50.0);
	rbgabuffer_LineTo(bitmap_context, 20.0, 40.0);
	rbgabuffer_LineTo(bitmap_context, 0.0, 50.0);
	rbgabuffer_ClosePath(bitmap_context);
	rbgabuffer_FillColor(bitmap_context, 0xFFFFf000);
	rbgabuffer_Fill(bitmap_context);

	rbgabuffer_FillText(bitmap_context, "Test 🐨 🐱‍🚀 sadf", 10, 16, 500);

	while(elix_os_window_handle_events(w) ) {
		if ( w->flags & EOE_WIN_CLOSE ) {
			elix_os_window_destroy(w);
			break;
		}
		//update_buffer_randomly(bitmap_context->memory);

		fps.update();
		elix_os_window_render(w);
		//elix_os_system_idle(16000);
	}

	elix_os_window_destroy( w );
	delete w;
	return RESULTS_UNKNOWN;
}




function_results test_elix_rgbabuffer() {
	rbgabuffer_context * bitmap_context = rbgabuffer_create_context(nullptr, {{40,20}} );

	rbgabuffer_BeginPath(bitmap_context);
	rbgabuffer_Rect(bitmap_context, 8, 8, 12,3);
	rbgabuffer_FillColor(bitmap_context, 0xFFFF00FF);
	rbgabuffer_Fill(bitmap_context);

	rbgabuffer_BeginPath(bitmap_context);
	rbgabuffer_MoveTo(bitmap_context, 4.0, 0.0);
	rbgabuffer_LineTo(bitmap_context, 8.0, 9.0);
	rbgabuffer_LineTo(bitmap_context, 4.0, 7.0);
	rbgabuffer_LineTo(bitmap_context, 0.0, 9.0);
	rbgabuffer_ClosePath(bitmap_context);
	rbgabuffer_FillColor(bitmap_context, 0xFFFFf000);
	rbgabuffer_Fill(bitmap_context);

	print_rgbabuffer(bitmap_context);

	return RESULTS_UNKNOWN;
}


function_results test_elix_cstring() {
	char testA[] = ".asdfhg8dhfjk459fg9!@#$%^&*( kxfgf-78546fdsgl;'][.";
	char testB[] = "1234567890ABCDEFGH"; //18
//	char messageB[2][8] = {"Failed", "Success"};

	const char sanitisedTestA[] = "asdfhg8dhfjk459fg9kxfgf-78546fdsgl][.";

	LOG_INDENT("Sanitise");
	LOG_INDENT("Before: %s length:" pZU "", testA, elix_cstring_length_stupid_c(testA));
	elix_cstring_sanitise(testA);
	LOG_INDENT(" After: %s length:" pZU "", testA, elix_cstring_length_stupid_c(testA));

	if ( !elix_cstring_equal(testA,sanitisedTestA) ) {
		LOG_INDENT("String is not sanitised.");
	}

	LOG_INDENT("has_suffix(\"adsadsadas\", \"das\"): %d", elix_cstring_has_suffix("adsadsadas", "das"));
	LOG_INDENT("has_suffix(\"adsadsadas\", \"qdas\"): %d", elix_cstring_has_suffix("adsadsadas", "qdas"));

	LOG_INDENT("elix_cstring_find_of(\"asdfhg8dhfjk459fg9kxfgf\", \"dhf\"): %d", elix_cstring_find_of("asdfhg8dhfjk459fg9kxfgf", "dhf", 0));


	char * leftsub = nullptr, * leftnegsub = nullptr,* midsub = nullptr, * midnegsub = nullptr, * rightsub = nullptr, * rightnegsub = nullptr;

	leftsub = elix_cstring_substr(testB, 5, SSIZE_MAX);
	leftnegsub = elix_cstring_substr(testB, -5, SSIZE_MAX);
	midsub = elix_cstring_substr(testB, 2, 5);
	midnegsub = elix_cstring_substr(testB, 2, -2);
	rightsub = elix_cstring_substr(testB, 0, 10);
	rightnegsub = elix_cstring_substr(testB, 0, -10);


	LOG_INDENT("String Used: %s", testB); // 18
	LOG_INDENT("Left Substr: %s [" pZU ":%d] from " pZD, leftsub, elix_cstring_length_stupid_c(leftsub), 13, 5);
	LOG_INDENT("Left with negSubstr: %s [" pZU ":%d] from " pZD, leftnegsub,elix_cstring_length_stupid_c(leftnegsub), 5,  -5);
	LOG_INDENT("Mid Substr: %s [" pZU ":%d] from %d with length " pZD, midsub, elix_cstring_length_stupid_c(midsub),5,  2, 5);
	LOG_INDENT("Mid with neg Substr: %s [" pZU ":%d] from %d with length " pZD, midnegsub, elix_cstring_length_stupid_c(midnegsub),14, 2, -2);
	LOG_INDENT("Right Substr: %s [" pZU ":%d] from %d with length " pZD, rightsub, elix_cstring_length_stupid_c(rightsub),10, 0, 10);
	LOG_INDENT("right with neg Substr: %s [" pZU ":%d] from %d with length " pZD, rightnegsub,  elix_cstring_length_stupid_c(rightnegsub),8, 0, -10);

	return RESULTS_UNKNOWN;
}

inline uint8_t elix_compare( const void * p1, const void * p2, size_t size ) {
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < size; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
	}
	return 1;
}

struct program_directory_test {
	char * (*func)(const elix_program_info * program_info, bool shared, const char * filename );
	char title[128];
	bool shared;
	char * filename;
	char expected[768];
};
static char program_directory_test_filename[] = "file444.txt";
program_directory_test program_directory_test_list[] = {
	{&elix_program_directory_documents, "Document Directory (Public)", true, nullptr, "/usr/share/ElixTestProgram/ElixTestProgram" },
	{&elix_program_directory_documents, "Document Directory (User)", false, nullptr, "" },
	{&elix_program_directory_documents, "Document File (Public)",  true, program_directory_test_filename, "/usr/share/ElixTestProgram/ElixTestProgram/file444.txt" },
	{&elix_program_directory_documents, "Document File (User)", false, program_directory_test_filename, "" },

	{&elix_program_directory_documents, "User Directory (Roaming)", true, nullptr },
	{&elix_program_directory_documents, "User Directory", false, nullptr },
	{&elix_program_directory_documents, "User File (Roaming)",  true, program_directory_test_filename },
	{&elix_program_directory_documents, "User File", false, program_directory_test_filename },
};

function_results test_elix_program() {
	size_t error_count = 0;
	char * buffer = nullptr;

	LOG_INDENT("User Name: %s", program_info.user);
	LOG_INDENT("Program Name: %s", program_info.program_name);
	LOG_INDENT("Version: %s", program_info.program_version);
	LOG_INDENT("Level: %s", program_info.program_version_level);
	LOG_INDENT("Pre-set directory: %s", program_info.program_directory);
	LOG_INDENT("Binary: %s in %s", program_info.path_executable.filename, program_info.path_executable.path);
	LOG_INDENT("");

	for (auto &&i : program_directory_test_list) {
		buffer = i.func(&program_info, i.shared, i.filename);
		error_count += !(buffer);
		LOG_INDENT("%s %s: %s - Expected: %s", elix_compare(buffer, i.expected, 768) ? "✅" : "❌", i.title, buffer, i.expected);
		NULLIFY(buffer);
	}

	buffer = elix_program_directory_resources(&program_info, nullptr, EPRD_DATA);
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Resource Directory (EPRD_DATA)", buffer);
	NULLIFY(buffer);

	buffer = elix_program_directory_resources(&program_info, nullptr, EPRD_PARENT_SHARE);
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Resource Directory (EPRD_PARENT_SHARE)", buffer);
	NULLIFY(buffer);
	
	buffer = elix_program_directory_resources(&program_info, nullptr, EPRD_SHARE);
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Resource Directory (EPRD_SHARE)", buffer);
	NULLIFY(buffer);

	buffer = elix_program_directory_resources(&program_info, nullptr, EPRD_GLOBAL);
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Resource Directory (EPRD_GLOBAL)", buffer);
	NULLIFY(buffer);

	buffer = elix_program_directory_resources(&program_info, nullptr, EPRD_AUTO);
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Resource Directory (EPRD_AUTO)", buffer);
	NULLIFY(buffer);

	buffer = elix_program_directory_resources(&program_info, "file666.txt", EPRD_AUTO);
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Resource File", buffer);
	NULLIFY(buffer);
	
	buffer = elix_program_directory_cache_file(&program_info, "file666.txt");
	error_count += !(buffer);
	LOG_INDENT("%s: %s", "Cache File", buffer);
	NULLIFY(buffer);

	return error_count ? RESULTS_ERROR : RESULTS_SUCCESS;
}

function_results test_elix_os_directory() {
	char * dir = elix_program_directory_resources(&program_info, nullptr, EPRD_AUTO);
	char * subdir = elix_program_directory_resources(&program_info, "TestDir", EPRD_AUTO);

	LOG_INDENT("'%s' is dir? %d", dir, elix_os_directory_is(dir, &program_consent));
	LOG_INDENT("'%s' is dir? %d", "C:/Users/", elix_os_directory_is("C:/Users/", &program_consent));
	LOG_INDENT("'%s' is dir? %d", "/usr/", elix_os_directory_is("/usr/", &program_consent));

	LOG_INDENT("elix_os_directory_make(subdir):  %d", elix_os_directory_make_stupid_c(subdir));
	LOG_INDENT("elix_os_directory_is(subdir):  %d", elix_os_directory_is(subdir, &program_consent));
	LOG_INDENT("elix_os_directory_remove(subdir):  %d", elix_os_directory_remove(subdir, false, &program_consent));
	LOG_INDENT("elix_os_directory_is(subdir):  %d", elix_os_directory_is(subdir, &program_consent));

	const char * directory_pth= ".";
	elix_directory * directory = elix_os_directory_list_create(directory_pth, nullptr, &program_consent);
	if ( directory ) {
		for (size_t b = 0; b < directory->count; ++b) {
			if ( elix_os_directory_is(directory->files[b].uri, nullptr) ) {
				LOG_INDENT("D: %s %s '%s'", directory->files[b].path, directory->files[b].filename, directory->files[b].uri);
			} else {
				LOG_INDENT("F: %s %s %s '%s'", directory->files[b].path, directory->files[b].filename, directory->files[b].filetype, directory->files[b].uri);
			}
		}
		elix_os_directory_list_destroy(&directory);
	}

	delete dir;
	delete subdir;

	return RESULTS_UNKNOWN;
}

#include "extra/elix_package.h"

function_results test_elix_package() {
	size_t before,  after;

	before = elix_os_memory_usage();

	elix_package * puttris = elix_package_create("bin/puttytris.game", EP_GAME_OLD);
	elix_package_info(puttris);

	elix_package_data data = elix_package_get_file(puttris, "./game.mokoi");

	LOG_INDENT("Content of ./game.mokoi");
	for (uint32_t y = 0; y < data.size; ++y) {
		printf("%c", data.data[y]);
	}
	printf("\n");

	elix_package_data data2 = elix_package_get_file(puttris, "./c/scripts/main.amx");

	LOG_INDENT("Content of main.amx");
	for (uint32_t y = 0; y < data2.size; ++y) {
		if( y % 64 ==0 && y )
			printf("\n");
		printf("%02x ", data2.data[y]);

	}
	printf("\n");

	elix_package_destroy(puttris);
	delete puttris;

	after = elix_os_memory_usage();
	LOG_INDENT("Memory Usage: Before: " pZU ", After: " pZU ", Diff: " pZU, before, after, after - before);
	return RESULTS_UNKNOWN;
}

const char * test_string_list[] = {
"SaddleLeg4020",
"SaltRainbow2251",
"SchoolGirl5862",
"KnifeMagnet8136",
"ClownPlanet8229",
"SoftwareMeteor2872",
"Chess BoardMilkshake3081",
"VacuumBarbecue8380",
"ClockVideotape4497",
"RadarMap2097",
"AirRadar3434",
"OnionAlbum6464",
"SnailTrain3015",
"CometClown4493",
"MosquitoBaby5980",
"PendulumAlbum3311",
"LibraryHat7864",
"PocketTiger5482",
"CometCircle3507",
"TunnelVulture8830",
"AirFamily7921",
"ExplosiveFan5140",
"X-RayAircraft Carrier7281",
"AirPrison3543",
"TriangleWoman7042",
"SandpaperAlphabet2502",
"FreewayBrain2379",
"BowlSoftware3966",
"MistSpoon4129",
"WaterAeroplane5497",
"PendulumSandwich8967",
"VideotapeHorse3842",
"LiquidCircus8543",
"ChiefTunnel3541",
"GardenInsect4471",
"BibleComet9128",
"FeatherChurch4292",
"SexSwimming Pool9581",
"SurveyorCompass8408",
"StarBible8530",
"RecordChocolates3557",
"CompassFreeway5837",
"RifleSurveyor2915",
"SpectrumPepper8152",
"CarpetTapestry1057",
"Data BasePocket6545",
"CarrotNavy6857",
"CircusFilm4308",
"BossPillow8214",
"SwordFinger7208",
"HorseIce1169",
"PocketFruit5775",
"AlphabetKaleidoscope1367",
"Space ShuttleFlower8972",
"ArmWorm7243",
"Jet FighterTorpedo5572",
"CarrotDung5589",
"PotatoStomach2842",
"RockCar-Race3676",
"VampireAeroplane7563",
"BibleSun5390",
"WindowTapestry1379",
"MeatIce6869",
"SandpaperBee9758",
"PlanetComet2590",
"BowlBox6295",
"MouthDesk6919",
"Compact DiscDress4703",
};


function_results test_elix_hash() {
	uint8_t tests_failed = 0;
	elix_hashmap * hm = elix_hashmap_create();

	for (uint16_t c = 0; c < 68; c++) {
		elix_hashmap_insert(hm, test_string_list[c], (data_pointer)test_string_list[c]);
	}

	for (uint16_t c = 0; c < 68; c++) {
		char * hashs = (char*)elix_hashmap_value(hm, test_string_list[c]);
		if ( !elix_cstring_equal(hashs, test_string_list[c]) ){
			LOG_INDENT("index %d doesn't match, it should be '%s' got '%s'", c, test_string_list[c], hashs);
			tests_failed++;
		}
	}

	elix_hashmap_remove(hm, "VampireAeroplane7563", nullptr);

	if ( elix_hashmap_value(hm, test_string_list[59]) != nullptr) {
		LOG_INDENT("VampireAeroplane7563 hasn't been removed");
		tests_failed++;
	}

	elix_hashmap_destroy(&hm, nullptr);
	LOG_INDENT("--- Errors: %u ------------------------------------------", tests_failed);
	return RESULTS_UNKNOWN;
}

function_results test_directory_watch() {
	int64_t timestamp = 0;
	uint8_t results = elix_file_modified_check("bin/a.txt", &timestamp);

	LOG_INDENT("%d: %s", results, ctime(&timestamp));
	while ( results > 0 ) {
		results = elix_file_modified_check("bin/a.txt", &timestamp);
		if ( results == 2 ) {
			LOG_INDENT("%d: %s", results, ctime(&timestamp));
		}
		elix_os_system_idle(1000);
	}
	return RESULTS_UNKNOWN;
}

function_results test_console() {
	LOG_INDENT("Console Test: %s\n", "🎒🤔🐱‍🚀" );
	return RESULTS_UNKNOWN;
}

typedef struct test_method {
	char name[64];
	function_results (*func)();
	function_results result;
	double time;
	size_t memory_change;
} test_method;

static test_method tests[] = {
	//{"Console", &test_console, RESULTS_PENDING, 0.0},
	{"Program Info", &test_elix_program, RESULTS_PENDING, 0.0},
	{"Directories", &test_elix_os_directory, RESULTS_PENDING, 0.0},
	//{"Endian", &test_elix_endian, RESULTS_PENDING, 0.0},
	{"HTML Parsing", &test_elix_html, RESULTS_PENDING, 0.0},
	//{"HTML Rendering", &test_elix_html_window, RESULTS_PENDING, 0.0},
	
	//{"RGBABuffer", &test_elix_rgbabuffer, RESULTS_PENDING, 0.0},

	//{"Hash table", &test_elix_hash, RESULTS_PENDING, 0.0},
	//{"Window/Canvas", &test_elix_os_window, RESULTS_PENDING, 0.0},
	//{"Directory Watcher", &test_directory_watch, RESULTS_PENDING, 0.0},
	//{"C Strings", &test_elix_cstring, RESULTS_PENDING, 0.0},

	//{"Packages", &test_elix_package, RESULTS_PENDING, 0.0},
};


void test_run_method( test_method & method ) {
	if ( method.func ) {
		LOG_TEXT("--- %s -----------------------------", method.name);
		struct timespec start, end;
		size_t before,  after;
		before = elix_os_memory_usage();
		clock_gettime(CLOCK_MONOTONIC, &start );
		method.result = method.func();
		clock_gettime(CLOCK_MONOTONIC, &end );
		method.time = difftime(end.tv_sec, start.tv_sec) + ((double)(end.tv_nsec - start.tv_nsec)/1.0e9);
		after = elix_os_memory_usage();
		method.memory_change = after - before;
	} else {
		method.result = RESULTS_FUNCTION_UNIMPLEMENTED;
	}
}

int main(int UNUSEDARG argc, char UNUSEDARG * argv[]) {
	program_info = elix_program_info_create(argv[0], "Elix Test Program", "0.4", nullptr);

	for (auto &&i : tests) {
		test_run_method(i);
	}
	
	LOG_HR();

	for (auto &&i : tests) {
		LOG_TEXT("[%s] 0x%x - Time: %f - Possible Memory Leak: " pZU "b", i.name, i.result, i.time, i.memory_change  );
	}

	return 0;
}

