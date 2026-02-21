#include "./elix/elix_cstring.h"
#include "./elix/elix_os.h"


#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"

typedef struct elix_font {
	stbtt_fontinfo info;
	elix_databuffer data;
	float scale, baseline;
	int base_ascent, base_descent, base_line_gap;
} elix_font;

void elix_font_destroy( elix_font * font) {
	NULLIFY(font->data.data);
	font->data.size = 0;
	NULLIFY(font);
}

elix_font * elix_font_create(const char * font_name) {
	elix_font * font = nullptr;
	elix_databuffer raw_file;

	raw_file = elix_os_font(font_name);
	if ( raw_file.size ) {
		font = ALLOCATE(elix_font, 1);
		font->data = raw_file;
		int index = stbtt_GetFontOffsetForIndex(raw_file.data, 0);
		if ( index != -1 ) {
			stbtt_InitFont(&font->info, raw_file.data, index);
			stbtt_GetFontVMetrics(&font->info, &font->base_ascent, &font->base_descent, &font->base_line_gap);
		}
	}

	return font;
}

elix_v64_2 elix_font_text_measure(const char * text, size_t text_length, elix_font * font, float font_size_px) {
	elix_v64_2 metrics = {0.0, 0.0};
	float font_scale = stbtt_ScaleForPixelHeight(&font->info, font_size_px);

	int ascent, descent;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, nullptr);
	int baseline = (int) (ascent * font_scale);
	int descent_scaled = (int) (descent * font_scale);

	char * object = (char*)text;
	uint32_t current_character = 0, next_character = 0;
	size_t count = 0;
	while ( (current_character = elix_cstring_next_character(object)) > 0 && count <text_length ) {
		next_character = elix_cstring_peek_character(object);
		count++;


		int x0, y0, x1, y1;
		int advance, lsb;

		stbtt_GetCodepointBitmapBox(&font->info, current_character, font_scale, font_scale, &x0, &y0, &x1, &y1);
		stbtt_GetCodepointHMetrics(&font->info, current_character, &advance, &lsb);

		metrics.width += advance * font_scale;
		if (next_character) {
			metrics.width += font_scale * stbtt_GetCodepointKernAdvance(&font->info, current_character, next_character);
		}

	}
	return metrics;
}