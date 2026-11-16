#include "elix_endian.h"
#include "elix_html.h"
#include "elix_json.h"
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
static elix_program_info test_program_info = {0};
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

elix_string_buffer elix_string_buffer_from_file(const char * requested_filename) {
	elix_string_buffer str = {0};
	elix_file file;
	size_t data_size = 0;
	if ( elix_file_open(&file, requested_filename, EFF_FILE_READ, nullptr) ) {
		str.string.data = elix_file_read_content(&file, &data_size );
		str.string.allocated = data_size;
		str.string.length = data_size;
		str.string.location = 0;

		str.iter = str.string.data;

		elix_file_close(&file);
	} else {
		LOG_ERROR("File not found: %s", requested_filename);
	}
	return str;
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

/*
const char * test_elix_json_string = R"TEXT({
"sortBy":null,"page":1,"totalProducts":439,"totalPages":5,"productsPerPage":100,"contentSystemCompatibility":null,"moviesCount":2,"tags":[
    {"id":"171944464","name":"COMPLETED","productCount":"0"},{"id":"124103914","name":"NEXT TO PLAY","productCount":"0"},{"id":"76263364","name":"BACKLOG","productCount":"0"},{"id":"28422814","name":"FAVORITE","productCount":"0"}
],"updatedProductsCount":176,"hiddenUpdatedProductsCount":7,"appliedFilters":{"tags":null},"hasHiddenProducts":true

})TEXT";*/

elix_json_token test_elix_json_tokens_results[] = {
		//{JSON_OBJECT, 0, 0, 0, 0, 124 },
		{JSON_OBJECT, 0, {0, 0}, {0, 121} },
		{JSON_ARRAY, 0, {2, 4}, {9, 16} },
		{JSON_NUMBER, 1, {0, 0}, {9, 1} },
		{JSON_STRING, 1, {0, 0}, {12, 8} },
		{JSON_NUMBER, 1, {0, 0}, {22, 3} },
		{JSON_ARRAY, 0, {29, 3}, {35, 50} },
		{JSON_OBJECT, 5, {0, 0}, {36, 15} },
		{JSON_STRING, 6, {37, 4}, {44, 6} },
		{JSON_OBJECT, 5, {0, 0}, {55, 15} },
		{JSON_STRING, 8, {56, 4}, {63, 6} },
		{JSON_OBJECT, 5, {0, 0}, {74, 10} },
		{JSON_ARRAY, 10, {75, 4}, {82, 1} },
		{JSON_NUMBER, 11, {0, 0}, {82, 1} },
		{JSON_PRIMITIVE, 0, {89, 6}, {98, 4} },
		//{JSON_PRIMITIVE, 0, 110, 2, 119, 4 },
		{JSON_PRIMITIVE, 0, {107, 2}, {116, 4} },
		{0,0,{0,0},{0,0}}

};

const char * test_elix_json_string = R"TEXT({"list":[1,"🐱🚀",254], "obj":[{"key1":"value2"}, {"key3":"value3"}, {"key2":[1]}], "Action": true,   "DF"    : null})TEXT";

const char * elix_json_token_mismatch_str[] = {"TYPE", "PARENT", "KEY", "DATA", ""};
const char * elix_json_token_mismatch(elix_json_token a, elix_json_token b ) {
	if ( a.type != b.type ) {
		return elix_json_token_mismatch_str[0];
	} else if ( a.parent != b.parent ) {
		return elix_json_token_mismatch_str[1];
	} else if ( a.key.offset != b.key.offset || a.key.length != b.key.length) {
		return elix_json_token_mismatch_str[2];
	} else if ( a.data.offset != b.data.offset || a.data.length != b.data.length) {
		return elix_json_token_mismatch_str[3];
	} else {
		return elix_json_token_mismatch_str[4];
	}
}

function_results test_elix_json() {
	uint16_t test_failed = 0, test_passed = 0;
	elix_string_buffer test_html = elix_string_buffer_new(test_elix_json_string, 512);
	elix_json json = elix_json_open(&test_html, 0);
	elix_json test_elix_json_results = {0, test_elix_json_tokens_results};

	LOG_INDENT("Source", "");
	LOG_PRINT("%.*s", test_html.string.length, test_html.string.data);
	LOG_INDENT("Parsed Results", "");

	elix_json_print(&json);

	test_elix_json_results.reference = &test_html;

	if ( test_elix_json_results.index != json.index ) {
		test_failed++;
		LOG_PRINT("Token Uses Mismatch ", ""); 
	}
	for (uint16_t i = 0; i <= json.index; i++) {
		if ( test_elix_json_results.tokens[i].type != json.tokens[i].type ) {
			test_failed++;
			LOG_PRINT("Token %d type-mismatch", i); 
		} else if ( test_elix_json_results.tokens[i].parent != json.tokens[i].parent ) {
			test_failed++;
			LOG_PRINT("Token %d parent-mismatch", i); 
		} else if ( test_elix_json_results.tokens[i].key.offset != json.tokens[i].key.offset || test_elix_json_results.tokens[i].key.length != json.tokens[i].key.length) {
			test_failed++;
			LOG_PRINT("Token %d key-mismatch", i); 
		} else if ( test_elix_json_results.tokens[i].data.offset != json.tokens[i].data.offset || test_elix_json_results.tokens[i].data.length != json.tokens[i].data.length) {
			test_failed++;
			LOG_PRINT("Token %d data-mismatch", i); 
		} else {
			test_passed++;
		}

	}
	LOG_HR();
	LOG_INDENT("Results: %d tests failed", test_failed);
	LOG_INDENT("         %d tests passed", test_passed);
	elix_string_buffer_delete(&test_html);

	elix_json_close(&json);

	if ( test_failed ) {
		return RESULTS_FAILED;
	} else {
		return RESULTS_SUCCESS;
	}
	
}


const char * test_elix_html_string = R"TEXT(<!DOCTYPE html><html>
	<!-- Commement --><body>Hello <![CDATA[ sdaghkl
 asd]] ]]> 🐨 World 🐱‍🚀<hr><div class="asdd" fail="this"><div tag>test</div>🐨🐨</div></body></html>)TEXT";

extern "C" elix_html_document * elix_html_new( elix_string_buffer * content);

function_results test_elix_html() {

	elix_string_buffer test_html = elix_string_buffer_new(test_elix_html_string, 512);
	LOG_INDENT("Source", "");
	LOG_INDENT("%.*s", test_html.string.length, test_html.string.data);

	elix_html_document * html = elix_html_new(&test_html);
	elix_html_print(html);
	elix_html_close(html);

	NULLIFY(html);


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

	rgbabuffer_delete_context(bitmap_context);

	elix_html_close(html);

	elix_rendertree_delete(&tree);

	NULLIFY(w);
	NULLIFY(bitmap_context);
	NULLIFY(html);
	return RESULTS_UNKNOWN;
}

function_results test_elix_os_window() {
	elix_os_window * w = elix_os_window_create({{600, 400}}, {4,4});

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );
	//update_buffer_randomly(bitmap_context->memory);
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
	update_buffer_randomly(bitmap_context->memory);
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

inline size_t elix_cstring_length_stupid_c(const char * string) {
	return elix_cstring_length(string, 0);
}

function_results test_elix_cstring() {
	char testA[] = ".asdfhg8dhfjk459fg9!@#$%^&*( kxfgf-78546fdsgl;'][.";
	char testB[] = "1234567890ABCDEFGH"; //18
//	char messageB[2][8] = {"Failed", "Success"};

	const char sanitisedTestA[] = "asdfhg8dhfjk459fg9kxfgf-78546fdsgl][.";

	LOG_INDENT("Sanitise");
	LOG_INDENT("Before: %s length:" pZU "", testA, elix_cstring_length(testA, 0));
	elix_cstring_sanitise(testA);
	LOG_INDENT(" After: %s length:" pZU "", testA, elix_cstring_length(testA, 0));

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
	if (!p1 || !p2)
		return 0;
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < size; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
	}
	return 1;
}

inline uint8_t elix_cstring_compare( const void * p1, const void * p2, size_t size ) {
	if (!p1 || !p2)
		return 0;
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < size; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
		if ( a[i] == 0 ) {
			return 1;
		}
	}
	return 1;
}

struct program_directory_test {
	uint32_t test;
	char title[128];
	char * filename;
	char expected[768];
};
static char program_directory_test_filename[] = "file444.txt";
program_directory_test program_directory_test_list[] = {
	//Based (argv[0], "Elix Test Program", "0.4", "beta"
	{ 0xf1, "Config Directory (Global)", nullptr, "/etc/ElixTestProgram-beta/" },
	{ 0x01, "Config Directory (User)", nullptr, "~/.config/ElixTestProgram-beta/" },
	{ 0xf1, "Config File (Global)", program_directory_test_filename, "/etc/ElixTestProgram-beta/file444.txt" },
	{ 0x01, "Config File (User)", program_directory_test_filename, "~/.config/ElixTestProgram-beta/file444.txt" },
	{ 0x02, "Document Directory", nullptr, "~/Documents/ElixTestProgram-beta/" },
	{ 0xf2, "Document File", program_directory_test_filename, "~/Documents/file444.txt" },
	{ 0x02, "Document Directory & File", program_directory_test_filename, "~/Documents/ElixTestProgram-beta/file444.txt" },
	{ 0xf3, "Resources Directory (Global)", nullptr, "/usr/share/ElixTestProgram-beta/" },
	{ 0x03, "Resources Directory (User)", nullptr, "~/.local/share/ElixTestProgram-beta/" },
	{ 0xD0, "Resources Directory (EPRD_GLOBAL)", nullptr, "/usr/share/ElixTestProgram-beta/" },
	{ 0xD1, "Resources Directory (EPRD_PARENT_SHARE)", nullptr, "{$cwd}/../share/ElixTestProgram-beta/" },
	{ 0xD2, "Resources Directory (EPRD_SHARE)", nullptr, "{$cwd}/share/ElixTestProgram-beta/" },
	{ 0xD3, "Resources Directory (EPRD_DATA)", nullptr, "{$cwd}/ElixTestProgram-beta_data/" },
	{ 0xD4, "Resources Directory (EPRD_PARENT_SHARE_MUTATABLE)", nullptr, "{$cwd}/../share/test-debug-linux-x86_64/" },
	{ 0xD5, "Resources Directory (EPRD_SHARE_MUTATABLE)", nullptr, "{$cwd}/share/test-debug-linux-x86_64/" },
	{ 0xD6, "Resources Directory (EPRD_DATA_MUTATABLE)", nullptr, "{$cwd}/test-debug-linux-x86_64_data/" },
	{ 0xf3, "Resources File (Global)", program_directory_test_filename, "/usr/share/ElixTestProgram-beta/file444.txt" },
	{ 0x03, "Resources File (User)", program_directory_test_filename, "~/.local/share/ElixTestProgram-beta/file444.txt" },
	{ 0x00, "Cache File", program_directory_test_filename, "~/.cache/ElixTestProgram-beta/file444.txt" },
	{ 0xF0, "Cache File", program_directory_test_filename, "/run/user/1000/ElixTestProgram-beta/file444.txt" },
};

extern "C" size_t elix_cstring_inreplace( char * source_text, size_t buffer_size, const char *search, const char *replace, uint8_t repeat);

function_results test_elix_program() {
	size_t error_count = 0;
	char * buffer = nullptr;

	LOG_INDENT("Running %s - Version %s", test_program_info.program_name, test_program_info.program_version);
	LOG_INDENT("Binary: %s in %s", test_program_info.path_executable.filename, test_program_info.path_executable.path);
	LOG_INDENT("System Directory Name: %s", test_program_info.program_directory);
	LOG_INDENT("Create from Program Name: %s", test_program_info.program_name);
	LOG_INDENT("     & Version Directory: %s", test_program_info.program_version_directory[0] ? test_program_info.program_version_directory : "[Unset]");
	LOG_INDENT("");
	LOG_INDENT("Running as User: %s", test_program_info.system_user);
	
	LOG_INDENT("");

	for (auto &&i : program_directory_test_list) {
		switch (i.test)	{
			case 0xf1: buffer = elix_program_directory_config( &test_program_info, i.filename, true, 1 ); break;
			case 0x01: buffer = elix_program_directory_config( &test_program_info, i.filename, true, 0 ); break;
			case 0xf2: buffer = elix_program_directory_document( &test_program_info, i.filename, true, 1 ); break;
			case 0x02: buffer = elix_program_directory_document( &test_program_info, i.filename, true, 0 ); break;
			case 0xf3: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_GLOBAL_SHARE ); break;
			case 0x03: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_LOCAL_SHARE ); break;
			case 0x00: buffer = elix_program_directory_cache( &test_program_info, i.filename, true, 0 ); break;
			case 0xf0: buffer = elix_program_directory_cache( &test_program_info, i.filename, true, 1 ); break;
			case 0xD0: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_GLOBAL_SHARE ); break;
			case 0xD1: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_PARENT_SHARE ); break;
			case 0xD2: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_SHARE ); break;
			case 0xD3: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_DATA ); break;
			case 0xD4: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_PARENT_SHARE_MUTATABLE ); break;
			case 0xD5: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_SHARE_MUTATABLE ); break;
			case 0xD6: buffer = elix_program_directory_resource( &test_program_info, i.filename, true, EPRD_DATA_MUTATABLE ); break;

		}
		error_count += !(buffer);
		if ( elix_cstring_has_prefix(buffer, test_program_info.path_executable.path) ) {
			elix_cstring_inreplace(buffer, ELIX_FILE_PATH_LENGTH, test_program_info.path_executable.path, "{$cwd}/", 0);
		}
		///Note: Yes, this is just a hack for my system only.
		if ( elix_cstring_has_prefix(buffer, "/home/luke/") ) {
			elix_cstring_inreplace(buffer, ELIX_FILE_PATH_LENGTH, "/home/luke/", "~/", 0);
		}
		
		LOG_INDENT("%s %s: %s - Expected: %s", elix_cstring_compare(buffer, i.expected, 768) ? "✅" : "❌", i.title, buffer, i.expected);
		NULLIFY(buffer);
	}

	return error_count ? RESULTS_ERROR : RESULTS_SUCCESS;
}

function_results test_elix_os_directory() {
	char * dir = elix_program_directory_resource(&test_program_info, nullptr, true, EPRD_LOCAL_SHARE);
	char * subdir = elix_program_directory_resource(&test_program_info, "TestDir", true, EPRD_LOCAL_SHARE);

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
	LOG_INDENT("Emoji: %s", "🎒🤔🐱‍🚀" );
	return RESULTS_SUCCESS;
}

typedef struct test_method {
	char name[64];
	function_results (*func)();
	function_results result;
	double time;
	size_t memory_change;
} test_method;

static test_method tests[] = {
	
	{"Console", &test_console, RESULTS_PENDING, 0.0},
	{"Program Info", &test_elix_program, RESULTS_PENDING, 0.0},
	//{"Directories", &test_elix_os_directory, RESULTS_PENDING, 0.0},
	//{"Endian", &test_elix_endian, RESULTS_PENDING, 0.0},
	//{"JSON Parsing", &test_elix_json, RESULTS_PENDING, 0.0},
	//{"HTML Parsing", &test_elix_html, RESULTS_PENDING, 0.0},

	//{"HTML Rendering", &test_elix_html_window, RESULTS_PENDING, 0.0},
	
	//{"RGBABuffer", &test_elix_rgbabuffer, RESULTS_PENDING, 0.0},

	//{"Hash table", &test_elix_hash, RESULTS_PENDING, 0.0},
	//{"Window/Canvas", &test_elix_os_window, RESULTS_PENDING, 0.0},

	//{"C Strings", &test_elix_cstring, RESULTS_PENDING, 0.0},

	/* Unfinished Code */
	//{"Directory Watcher", &test_directory_watch, RESULTS_PENDING, 0.0},
	//{"Packages", &test_elix_package, RESULTS_PENDING, 0.0},
};


void test_run_method( test_method & method ) {
	if ( method.func ) {
		LOG_TEXT("--- %s ----------------------------------------", method.name);
		struct timespec start, end;
		size_t before, after;
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
	test_program_info = elix_program_info_create(argv[0], "Elix Test Program", "0.4", "beta");

	for (auto &&i : tests) {
		test_run_method(i);
		LOG_HR();
	}

	for (auto &&i : tests) {
		LOG_TEXT("%s %s - Time: %f - Possible Memory Leak: " pZU "%s", i.result == RESULTS_SUCCESS ? "✅" : "❌", i.name, i.time, i.memory_change > 1024 ? i.memory_change / 1024 : i.memory_change, i.memory_change > 1024 ? "kb" : "b"   );
	}

	return 0;
}

