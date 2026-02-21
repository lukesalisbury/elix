#include "elix_endian.hpp"
#include "elix_html.hpp"
#include "extra/elix_fpscounter.hpp"
#include "window/elix_os_window.hpp"

#include "elix_rgbabuffer.hpp"

#include "elix_cstring.hpp"
#include "elix_os.hpp"
#include "elix_graphics.hpp"
#include "elix_file.hpp"
static elix_program_info program_info;


union pixel_4bpp {

	uint32_t raw;
	struct { // Note: CLANG  -Wno-gnu-anonymous-struct
		uint8_t x0:4;
		uint8_t x1:4;
		uint8_t x2:4;
		uint8_t x3:4;
		uint8_t x4:4;
		uint8_t x5:4;
		uint8_t x6:4;
		uint8_t x7:4;
	};
};

uint16_t merge_plane_8(uint8_t a, uint8_t b) {
	uint16_t value = 0;
	for ( uint8_t x = 0; x < 8; x++) {
		value |= ((a >> x) & 1) << (x*2);
		value |= ((b >> x) & 1) << (x*2)+1;
	}
	return value;
}


uint32_t merge_plane_16(uint16_t a, uint16_t b) {
	uint32_t value = 0;
	for ( uint8_t x = 0; x < 8; x++) {
		value |= ((a >> x*2) & 0b11) << (x*4);
		value |= ((b >> x*2) & 0b11) << (x*4)+2;
	}
	return value;
}

uint32_t load_image(char * file, rbgabuffer_context * bitmap_context ) {

	elix_file image;
	elix_file palette;

	elix_colour loaded_palette[32] = {
		0x00FFFF00, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
		0xFFFFFFFF, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF,
	 };

	if (elix_file_open(&image, "bin/pixel_test.pal") ) {
		file_size data_size = 0;
		uint8_t * data = elix_file_read_content(&image, data_size);
		uint16_t temp_colour = 0;
		LOG_INFO("PAL");
		for (file_size i = 0; i < data_size; i+= 2) {
			elix_colour colour;
			temp_colour = data[i] | data[i+1] <<8 ;
			
			colour.b = ((temp_colour) % 32) * 8;
			colour.g = ((temp_colour/32) % 32) * 8;
			colour.r = ((temp_colour/1024) % 32) * 8;

			loaded_palette[i/2] = colour;

			rbgabuffer_BeginPath(bitmap_context);
			rbgabuffer_Rect(bitmap_context, 4, 8 + (8*i), 16, 16);
			rbgabuffer_FillColor(bitmap_context, colour.hex);
			rbgabuffer_Fill(bitmap_context);

		}
		elix_file_close(&image);
	}

	if (elix_file_open(&image, "bin/pixel_test.pic") ) {
		file_size data_size = 0;
		uint8_t * data = elix_file_read_content(&image, data_size);
		LOG_INFO("PIC");

		for (file_size i = 0; i < data_size; i+=4) {
			
			pixel_4bpp pixel_row = {0};
			//uint16_t planeA = merge_plane_8(data[i], data[i+1]);
			//uint16_t planeB = merge_plane_8(data[i+2], data[i+3]);
			uint16_t planeA = merge_plane_8( data[i], data[i+1]);
			uint16_t planeB = merge_plane_8(data[i+2], data[i+3]);
			pixel_row.raw = merge_plane_16(planeA,planeB);
			/*
			[r0, bp1], [r0, bp2], [r1, bp1], [r1, bp2], [r2, bp1], [r2, bp2], [r3, bp1], [r3, bp2]
  [r4, bp1], [r4, bp2], [r5, bp1], [r5, bp2], [r6, bp1], [r6, bp2], [r7, bp1], [r7, bp2]
  [r0, bp3], [r0, bp4], [r1, bp3], [r1, bp4], [r2, bp3], [r2, bp4], [r3, bp3], [r3, bp4]
  [r4, bp3], [r4, bp4], [r5, bp3], [r5, bp4], [r6, bp3], [r6, bp4], [r7, bp3], [r7, bp4]
			
			*/


			uint32_t y = i/4;
			uint8_t x = 0;
			uint32_t p = 0;
			//for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					p = (pixel_row.raw >> x*4) & 0b1111;
					rbgabuffer_BeginPath(bitmap_context);
					rbgabuffer_Rect(bitmap_context, 24 + x, 8 + y, 1, 1);
					rbgabuffer_FillColor(bitmap_context, loaded_palette[p].hex);
					rbgabuffer_Fill(bitmap_context);
				}
			//}

		}
		
		elix_file_close(&image);
	}



	return 0;
}


void __gridpattern(elix_graphic_data * buffer) {
	uint32_t * p = buffer->pixels;
	uint32_t a[2] { 0xFFEEEEEE, 0xFFAAAAAA };
	for ( uint32_t q = 0; q < buffer->pixel_count; q++, p++ ) {
		uint32_t l = (q / (buffer->width * 8));
		uint32_t x = (q % buffer->width) / 8;
		*p = a[(l+x)%2];
	}
}


int main(int UNUSEDARG argc, char UNUSEDARG * argv[])
{

	elix_os_window * w = elix_os_window_create({{800, 600}}, {4,4}, "Pixel Editor");

	rbgabuffer_context * bitmap_context = rbgabuffer_create_context( w->display_buffer, w->dimension );

	__gridpattern(bitmap_context->memory);

	load_image(nullptr, bitmap_context);


	while(elix_os_window_handle_events(w) ) {
		if ( w->flags & EOE_WIN_CLOSE ) {
			elix_os_window_destroy(w);
			break;
		}
		//update_buffer_randomly(bitmap_context->memory);

		elix_os_window_render(w);
		//elix_os_system_idle(16000);
	}
	
	elix_os_window_destroy( w );
	delete w;

	return 0;
}



