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

uint32_t elix_rendertree_to_rgbabuffer(elix_rendertree * tree, rbgabuffer_context * ctx, uint8_t redraw_all);


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


void test_elix_endian() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOG_MESSAGE("--- Elix Endian ----------------------------------------");

	uint32_t ul = 0x12030456;
	uint32_t uln = elix_endian_network32(ul);
	uint32_t ulh = elix_endian_host32(uln);
	LOG_MESSAGE("elix::endian:native 0x%08x\n", ul);
	LOG_MESSAGE("elix::endian:net32  0x%08x\n", uln);
	LOG_MESSAGE("elix::endian:host32 0x%08x\n", ulh);


	uint16_t us = 0x1203;
	uint16_t usn = elix_endian_network16(us);
	uint16_t ush = elix_endian_host16(usn);
	LOG_MESSAGE("elix::endian:native 0x%04x\n", us);
	LOG_MESSAGE("elix::endian:net16  0x%04x\n", usn);
	LOG_MESSAGE("elix::endian:host16 0x%04x\n", ush);

	LOG_MESSAGE("--------------------------------------------------------");
}


void test_elix_rendertree() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOG_MESSAGE("--- Elix Rendertree ------------------------------------");

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
	elix_rendertree tree = elix::html::build_render_tree(&html, bitmap_context->dimensions);
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
}

void test_elix_os_window() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOG_MESSAGE("--- Elix OS Window -------------------------------------");

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
}

void test_elix_rgbabuffer() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOG_MESSAGE("--- Elix RGBA Buffer -----------------------------------");
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

	printf("   0%*c%d\n", bitmap_context->memory->width-2, ' ', bitmap_context->memory->width  );
	uint32_t colour;
	for (uint32_t y = 0; y < bitmap_context->memory->height; ++y) {
		printf("%02d:", y);
		for (uint32_t x = 0; x < bitmap_context->memory->width; ++x) {
			//TODO Redo this
			colour = rbgabuffer_get_pixel(bitmap_context, x, y);
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

}


void test_elix_cstring() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOGF_MESSAGE("--- Elix C-String --------------------------------------");
	char testA[] = ".asdfhg8dhfjk459fg9!@#$%^&*( kxfgf-78546fdsgl;'][.";
	char testB[] = "1234567890ABCDEFGH"; //18
//	char messageB[2][8] = {"Failed", "Success"};

	const char sanitisedTestA[] = "asdfhg8dhfjk459fg9kxfgf-78546fdsgl][.";

	LOG_MESSAGE("Sanitise");
	LOG_MESSAGE("Before: %s length:" pZU "", testA, elix_cstring_length(testA));
	elix_cstring_sanitise(testA);
	LOG_MESSAGE(" After: %s length:" pZU "", testA, elix_cstring_length(testA));

	if ( !elix_cstring_equal(testA,sanitisedTestA) ) {
		LOG_MESSAGE("String is not sanitised.");
	}

	LOG_MESSAGE("has_suffix(\"adsadsadas\", \"das\"): %d", elix_cstring_has_suffix("adsadsadas", "das"));
	LOG_MESSAGE("has_suffix(\"adsadsadas\", \"qdas\"): %d", elix_cstring_has_suffix("adsadsadas", "qdas"));

	LOG_MESSAGE("elix_cstring_find_of(\"asdfhg8dhfjk459fg9kxfgf\", \"dhf\"): %d", elix_cstring_find_of("asdfhg8dhfjk459fg9kxfgf", "dhf"));


	char * leftsub = nullptr, * leftnegsub = nullptr,* midsub = nullptr, * midnegsub = nullptr, * rightsub = nullptr, * rightnegsub = nullptr;

	leftsub = elix_cstring_substr(testB, 5);
	leftnegsub = elix_cstring_substr(testB, -5);
	midsub = elix_cstring_substr(testB, 2, 5);
	midnegsub = elix_cstring_substr(testB, 2, -2);
	rightsub = elix_cstring_substr(testB, 0, 10);
	rightnegsub = elix_cstring_substr(testB, 0, -10);


	LOG_MESSAGE("String Used: %s", testB); // 18
	LOG_MESSAGE("Left Substr: %s [" pZU ":%d] from " pZD, leftsub, elix_cstring_length(leftsub), 13, 5);
	LOG_MESSAGE("Left with negSubstr: %s [" pZU ":%d] from " pZD, leftnegsub,elix_cstring_length(leftnegsub), 5,  -5);
	LOG_MESSAGE("Mid Substr: %s [" pZU ":%d] from %d with length " pZD, midsub, elix_cstring_length(midsub),5,  2, 5);
	LOG_MESSAGE("Mid with neg Substr: %s [" pZU ":%d] from %d with length " pZD, midnegsub, elix_cstring_length(midnegsub),14, 2, -2);
	LOG_MESSAGE("Right Substr: %s [" pZU ":%d] from %d with length " pZD, rightsub, elix_cstring_length(rightsub),10, 0, 10);
	LOG_MESSAGE("right with neg Substr: %s [" pZU ":%d] from %d with length " pZD, rightnegsub,  elix_cstring_length(rightnegsub),8, 0, -10);
}

void test_elix_program() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOGF_MESSAGE("--- Elix Program ---------------------------------------");
	LOG_MESSAGE("User: %s", program_info.user);
	LOG_MESSAGE("Name: %s", program_info.program_name);
	LOG_MESSAGE("Version: %s", program_info.program_version);
	LOG_MESSAGE("Level: %s", program_info.program_version_level);
	LOG_MESSAGE("Pre-set directory: %s", program_info.program_directory);
	LOG_MESSAGE("Binary: %s in %s", program_info.path_executable.filename, program_info.path_executable.path);


	LOG_MESSAGE("Document Directory (Public): %s", elix_program_directory_documents(&program_info, true));
	LOG_MESSAGE("Document Directory (User): %s", elix_program_directory_documents(&program_info, false));
	LOG_MESSAGE("Document File (Public): %s", elix_program_directory_documents(&program_info, true, "file333.txt"));
	LOG_MESSAGE("Document File (User): %s", elix_program_directory_documents(&program_info, false, "file222.txt"));
	LOG_MESSAGE("User Directory (Roaming): %s", elix_program_directory_user(&program_info, true));
	LOG_MESSAGE("User Directory: %s", elix_program_directory_user(&program_info, false));
	LOG_MESSAGE("User File (Roaming): %s", elix_program_directory_user(&program_info, true, "file444.txt"));
	LOG_MESSAGE("User File: %s", elix_program_directory_user(&program_info, false, "file555.txt"));

	LOG_MESSAGE("Resource Directory: %s", elix_program_directory_resources(&program_info));
	LOG_MESSAGE("Resource File: %s", elix_program_directory_resources(&program_info, "file666.txt"));

	LOG_MESSAGE("Cache File: %s", elix_program_directory_cache_file(&program_info, "file778.txt"));

}

void test_elix_os_directory() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOGF_MESSAGE("--- Elix Directory -------------------------------------");

	char * dir = elix_program_directory_resources(&program_info);
	char * subdir = elix_program_directory_resources(&program_info, "TestDir");
	LOG_MESSAGE("'%s' is dir? %d", dir, elix_os_directory_is(dir));
	LOG_MESSAGE("'%s' is dir? %d", "C:/Users/", elix_os_directory_is("C:/Users/"));
	LOG_MESSAGE("'%s' is dir? %d", "/usr/", elix_os_directory_is("/usr/"));

	LOG_MESSAGE("elix_os_directory_make(subdir):  %d", elix_os_directory_make(subdir));
	LOG_MESSAGE("elix_os_directory_is(subdir):  %d", elix_os_directory_is(subdir));
	LOG_MESSAGE("elix_os_directory_remove(subdir):  %d", elix_os_directory_remove(subdir));
	LOG_MESSAGE("elix_os_directory_is(subdir):  %d", elix_os_directory_is(subdir));

	delete dir;
	delete subdir;

}

#include "elix_package.hpp"

void test_elix_package() {

	LOG_MESSAGE("--------------------------------------------------------");
	LOGF_MESSAGE("--- Elix Packages --------------------------------------");

	size_t before,  after;

	before = elix_os_memory_usage();


	elix_package * puttris = elix_package_create("bin/puttytris.game", EP_GAME_OLD);
	elix_package_info(puttris);

	elix_package_data data = elix_package_get_file(puttris, "./game.mokoi");

	LOG_MESSAGE("Content of ./game.mokoi");
	for (uint32_t y = 0; y < data.size; ++y) {
		printf("%c", data.data[y]);
	}
	printf("\n");

	elix_package_data data2 = elix_package_get_file(puttris, "./c/scripts/main.amx");

	LOG_MESSAGE("Content of main.amx");
	for (uint32_t y = 0; y < data2.size; ++y) {
		if( y % 64 ==0 && y )
			printf("\n");
		printf("%02x ", data2.data[y]);

	}
	printf("\n");

	elix_package_destroy(puttris);
	delete puttris;

	after = elix_os_memory_usage();
	LOG_MESSAGE("Memory Usage: Before: " pZU ", After: " pZU ", Diff: " pZU, before, after, after - before);
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

#include "elix_hashmap.hpp"
void test_elix_hash() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOGF_MESSAGE("--- Elix Hash ------------------------------------------");
	uint8_t tests_failed = 0;
	elix_hashmap * hm = elix_hashmap_create();

	for (uint16_t c = 0; c < 68; c++) {
		elix_hashmap_insert(hm, test_string_list[c], (data_pointer)test_string_list[c]);
	}

	for (uint16_t c = 0; c < 68; c++) {
		char * hashs = (char*)elix_hashmap_value(hm, test_string_list[c]);
		if ( !elix_cstring_equal(hashs, test_string_list[c]) ){
			LOG_MESSAGE("index %d doesn't match, it should be '%s' got '%s'", c, test_string_list[c], hashs);
			tests_failed++;
		}
	}

	elix_hashmap_remove(hm, "VampireAeroplane7563");

	if ( elix_hashmap_value(hm, test_string_list[59]) != nullptr) {
		LOG_MESSAGE("VampireAeroplane7563 hasn't been removed");
		tests_failed++;
	}

	elix_hashmap_destroy(&hm);
	LOGF_MESSAGE("--- Errors: %u ------------------------------------------", tests_failed);
}



void test_directory_watch() {
	LOG_MESSAGE("--------------------------------------------------------");
	LOGF_MESSAGE("--- Elix File Watcher -----------------------------");

	int64_t timestamp = 0;
	uint8_t results = elix_file_modified_check("bin/a.txt", timestamp);
	LOG_MESSAGE("%d: %s", results, ctime(&timestamp));
	while ( results > 0 ) {
		results = elix_file_modified_check("bin/a.txt", timestamp);
		if ( results == 2 ) {
			LOG_MESSAGE("%d: %s", results, ctime(&timestamp));
		}
		elix_os_system_idle(1000);
	}
	LOGF_MESSAGE("--------------------------------------------------------");
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
	//test_run("Program Info", &test_elix_program);
	//test_run("Endian", &test_elix_endian);
	test_run("Rendertree", &test_elix_html);
	//test_run("Hash table", &test_elix_hash);

	//test_run("CANVAS", &test_elix_os_window);

	//test_run("Directory Watcher", &test_directory_watch);
	//test_elix_rgbabuffer();

	//test_elix_cstring();

	//

	//test_elix_os_directory();
	//test_elix_package();

	return 0;
}



