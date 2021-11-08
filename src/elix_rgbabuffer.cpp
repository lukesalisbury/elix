#include "elix_core.h"
#include "elix_rgbabuffer.hpp"
#include "elix_file.hpp"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"


struct rgbabuffer_edge_type {
	float ymin, ymax, xval, slope;
};
struct rgbabuffer_active_edge_index {
	uint32_t index;
	float xval;
};

struct rgbabuffer_font {
	stbtt_fontinfo info;
	elix_databuffer data;
	float scale, baseline;
	int base_ascent, base_descent, base_line_gap;
};


uint32_t rbgabuffer_get_pixel(rbgabuffer_context * ctx, uint32_t x , uint32_t y) {

	if (x < ctx->memory->width && y < ctx->memory->height ) {
		return ctx->memory->pixels[ x + (y*ctx->memory->width)];
	}
	return 0xDEADC0DE;
}

void rbgabuffer__pixel(rbgabuffer_context * ctx, uint32_t colour, int32_t x , int32_t y, UNUSEDARG float alpha = 1.0) {
	elix_colour final_colour = {colour};
	if ( alpha < 0.2 ) {
		return;
	}

	if ( alpha < 1.0 ) {
		elix_colour source_colour = {rbgabuffer_get_pixel(ctx,x,y)};
		final_colour.a = 255;
		final_colour.r = static_cast<uint8_t>( (final_colour.r * alpha) + (source_colour.r * (1.0 - alpha)) );
		final_colour.g = static_cast<uint8_t>( (final_colour.g * alpha) + (source_colour.g * (1.0 - alpha)) );
		final_colour.b = static_cast<uint8_t>( (final_colour.b * alpha) + (source_colour.b * (1.0 - alpha)) );

	}
	if (x >= 0 && y >= 0 && (uint32_t)x < ctx->memory->width && (uint32_t)y < ctx->memory->height ) {
		ctx->memory->pixels[ x + (y*ctx->memory->width)] = final_colour.hex;
	}
}

void rbgabuffer__line(elix_v2 p0, elix_v2 p1, rbgabuffer_context * ctx, uint32_t colour) {
	//ctx->memory[x+(y*ctx->width)] = colour;

	int32_t x0 = p0.x;
	int32_t x1 = p1.x;
	int32_t y0 = p0.y;
	int32_t y1 = p1.y;
	int32_t dy = y1 - y0;
	int32_t dx = x1 - x0;
	int32_t stepx, stepy;

	if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
	if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }

	rbgabuffer__pixel(ctx, colour, x0 ,y0 );
	#define myPixel( d, a, b, c) rbgabuffer__pixel(a, d, b ,c );
	if (dx > dy) {
		int length = (dx - 1) >> 2;
		int extras = (dx - 1) & 3;
		int incr2 = (dy << 2) - (dx << 1);
		if (incr2 < 0) {
			int c = dy << 1;
			int incr1 = c << 1;
			int d =  incr1 - dx;
			for (int i = 0; i < length; i++) {
				x0 += stepx;
				x1 -= stepx;
				if (d < 0) {
					rbgabuffer__pixel(ctx, colour, x0, y0 );
					rbgabuffer__pixel(ctx, colour, x0 += stepx, y0 );
					rbgabuffer__pixel(ctx, colour, x1, y1 );
					rbgabuffer__pixel(ctx, colour, x1 -= stepx, y1 );

					d += incr1;
				} else {
					if (d < c) {
						rbgabuffer__pixel(ctx, colour, x0, y0 );
						rbgabuffer__pixel(ctx, colour, x0 += stepx, y0 += stepy );
						rbgabuffer__pixel(ctx, colour, x1, y1 );
						rbgabuffer__pixel(ctx, colour, x1 -= stepx, y1 -= stepy );
					} else {
						rbgabuffer__pixel(ctx, colour, x0, y0 += stepy );
						rbgabuffer__pixel(ctx, colour, x0 += stepx, y0 );
						rbgabuffer__pixel(ctx, colour, x1, y1 -= stepy );
						rbgabuffer__pixel(ctx, colour, x1 -= stepx, y1 );
					}
					d += incr2;
				}
			}
			if (extras > 0) {
				if (d < 0) {
					myPixel( colour, ctx, x0 += stepx, y0);
					if (extras > 1)
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1);
				} else
				if (d < c) {
					myPixel( colour, ctx, x0 += stepx, y0);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1);
				} else {
					myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0);
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
				}
			}
		} else {
			int c = (dy - dx) << 1;
			int incr1 = c << 1;
			int d =  incr1 + dx;
			for (int i = 0; i < length; i++) {
				x0 += stepx;
				x1 -= stepx;
				if (d > 0) {
					myPixel( colour, ctx, x0, y0 += stepy);                      // Pattern:
					myPixel( colour, ctx, x0 += stepx, y0 += stepy);             //      o
					myPixel( colour, ctx, x1, y1 -= stepy);                      //    o
					myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);	          //  x
					d += incr1;
				} else {
					if (d < c) {
						myPixel( colour, ctx, x0, y0);                           // Pattern:
						myPixel( colour, ctx, x0 += stepx, y0 += stepy);         //      o
						myPixel( colour, ctx, x1, y1);                           //  x o
						myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);         //
					} else {
						myPixel( colour, ctx, x0, y0 += stepy);                  // Pattern:
						myPixel( colour, ctx, x0 += stepx, y0);                  //    o o
						myPixel( colour, ctx, x1, y1 -= stepy);                  //  x
						myPixel( colour, ctx, x1 -= stepx, y1);                  //
					}
					d += incr2;
				}
			}
			if (extras > 0) {
				if (d > 0) {
					myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
				} else
				if (d < c) {
					myPixel( colour, ctx, x0 += stepx, y0);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1);
				} else {
					myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0);
					if (extras > 2) {
						if (d > c) {
							myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
						} else {
							myPixel( colour, ctx, x1 -= stepx, y1);
						}
					}
				}
			}
		}
	} else {
		int length = (dy - 1) >> 2;
		int extras = (dy - 1) & 3;
		int incr2 = (dx << 2) - (dy << 1);
		if (incr2 < 0) {
			int c = dx << 1;
			int incr1 = c << 1;
			int d =  incr1 - dy;
			for (int i = 0; i < length; i++) {
				y0 += stepy;
				y1 -= stepy;
				if (d < 0) {
					myPixel( colour, ctx, x0, y0);
					myPixel( colour, ctx, x0, y0 += stepy);
					myPixel( colour, ctx, x1, y1);
					myPixel( colour, ctx, x1, y1 -= stepy);
					d += incr1;
				} else {
					if (d < c) {
						myPixel( colour, ctx, x0, y0);
						myPixel( colour, ctx, x0 += stepx, y0 += stepy);
						myPixel( colour, ctx, x1, y1);
						myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
					} else {
						myPixel( colour, ctx, x0 += stepx, y0);
						myPixel( colour, ctx, x0, y0 += stepy);
						myPixel( colour, ctx, x1 -= stepx, y1);
						myPixel( colour, ctx, x1, y1 -= stepy);
					}
					d += incr2;
				}
			}
			if (extras > 0) {
				if (d < 0) {
					myPixel( colour, ctx, x0, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1, y1 -= stepy);
				} else
				if (d < c) {
					myPixel( colour, ctx, stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1, y1 -= stepy);
				} else {
					myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
				}
			}
		} else {
			int c = (dx - dy) << 1;
			int incr1 = c << 1;
			int d =  incr1 + dy;
			for (int i = 0; i < length; i++) {
				y0 += stepy;
				y1 -= stepy;
				if (d > 0) {
					myPixel( colour, ctx, x0 += stepx, y0);
					myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					myPixel( colour, ctx, x1 -= stepx, y1);
					myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
					d += incr1;
				} else {
					if (d < c) {
						myPixel( colour, ctx, x0, y0);
						myPixel( colour, ctx, x0 += stepx, y0 += stepy);
						myPixel( colour, ctx, x1, y1);
						myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
					} else {
						myPixel( colour, ctx, x0 += stepx, y0);
						myPixel( colour, ctx, x0, y0 += stepy);
						myPixel( colour, ctx, x1 -= stepx, y1);
						myPixel( colour, ctx, x1, y1 -= stepy);
					}
					d += incr2;
				}
			}
			if (extras > 0) {
				if (d > 0) {
					myPixel( colour,ctx,x0 += stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
				} else
				if (d < c) {
					myPixel( colour,ctx,x0, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0 += stepx, y0 += stepy);
					if (extras > 2) myPixel( colour, ctx, x1, y1 -= stepy);
				} else {
					myPixel( colour,ctx,x0 += stepx, y0 += stepy);
					if (extras > 1) myPixel( colour, ctx, x0, y0 += stepy);
					if (extras > 2) {
						if (d > c) {
							myPixel( colour, ctx, x1 -= stepx, y1 -= stepy);
						}
						else {
							myPixel( colour, ctx, x1, y1 -= stepy);
						}
					}
				}
			}
		}
	}

}

void rbgabuffer__appendCommands(rbgabuffer_context * ctx, float * vals, uint32_t nvals) {

	if ( ctx->commands.index + nvals > ctx->commands.max ) {
		size_t ccommands = (ctx->commands.index + nvals + 16) * sizeof(float);
		float * ncommands = (float*)realloc(ctx->commands.array, ccommands);
		if (ncommands == nullptr) return; // Failure to create new array
		ctx->commands.array = ncommands;
		ctx->commands.max += ccommands;
	}

	memcpy(&ctx->commands.array[ctx->commands.index], vals, nvals*sizeof(float));

	ctx->commands.index += nvals;
}

void rbgabuffer__testpattern(elix_graphic_data * buffer) {
	uint32_t * p = buffer->pixels;
	uint32_t a[6] { 0xFF550000, 0xFF550055, 0xFF000055, 0xFF005555,0xFF005500,0xFF555500  };
	for ( uint32_t q = 0; q < buffer->pixel_count; q++, p++ ) {
		uint32_t l = q / buffer->width;
		*p = a[(l/10)%6];
	}
}

void rbgabuffer__gridpattern(elix_graphic_data * buffer) {
	uint32_t * p = buffer->pixels;
	uint32_t a[2] { 0xFFEEEEEE, 0xFFAAAAAA };
	for ( uint32_t q = 0; q < buffer->pixel_count; q++, p++ ) {
		uint32_t l = (q / (buffer->width * 64));
		uint32_t x = (q % buffer->width) / 64;
		*p = a[(l+x)%2];
	}
}

void rbgabuffer__staticpattern(elix_graphic_data * buffer) {
	uint32_t * p = buffer->pixels;
	uint32_t c = 0xFFFFFFFF;
	for ( uint32_t q = 0; q < buffer->pixel_count; q++, p++ ) {
		#if RAND_MAX == 32767
			c = (rand() * rand()) | 0xFF000000;
		#else
			c = rand() | 0xFF000000;
		#endif
		*p = c;
	}

}




void rbgabuffer_BeginPath(rbgabuffer_context* ctx) {
	ctx->commands.index = 0;
}

void rbgabuffer_ClosePath(rbgabuffer_context* ctx) {
	float vals[] = {
		rbgabuffer_CLOSE,
	};
	rbgabuffer__appendCommands(ctx, vals, ARRAYCOUNT(vals));
}

void rbgabuffer_Rect(rbgabuffer_context* ctx, float x, float y, float w, float h) {
	float vals[] = {
		rbgabuffer_MOVETO, x,y,
		rbgabuffer_LINETO, x,y+h,
		rbgabuffer_LINETO, x+w,y+h,
		rbgabuffer_LINETO, x+w,y,
		rbgabuffer_CLOSE
	};
	rbgabuffer__appendCommands(ctx, vals, ARRAYCOUNT(vals));
}


inline uint32_t insertSorted(rgbabuffer_active_edge_index * arr, rgbabuffer_active_edge_index key, size_t count, size_t capacity)
{
	if ( count >= capacity )
		return UINT32_MAX; //ERROR

	int32_t i = 0;
	if ( count ) {
		for (i=count-1; i >= 0 && arr[i].xval > key.xval; i--) {
			arr[i+1] = arr[i];
		}
		arr[i+1] = key;
	} else {
		arr[0] = key;
	}
	return i;
}

inline uint32_t rgbabuffer_edge_type_insert(rgbabuffer_edge_type * arr, rgbabuffer_edge_type key, size_t count, size_t capacity)
{
	if ( count >= capacity )
		return UINT32_MAX; //ERROR

	int32_t i = 0;
	if ( count ) { //
		//&& (arr[i].ymin > key.ymin || (arr[i].ymin == key.ymin && arr[i].xval > key.xval))
		for (i=count-1; i >= 0 && arr[i].ymin > key.ymin; i--) {
			arr[i+1] = arr[i];
		}
		arr[i+1] = key;
	} else {
		arr[0] = key;
	}
	return i;
}


void rbgabuffer_Fill(rbgabuffer_context* ctx) {
	float x = 0.0, y = 0.0;
	float * p = nullptr;
	elix_v2 points[64];
	uint32_t point_count = 0;
	size_t ind = 0;
	while (ind < ctx->commands.index) {
		uint32_t cmd = (uint32_t)ctx->commands.array[ind];
		switch (cmd) {
		case rbgabuffer_MOVETO:
			p = &ctx->commands.array[ind+1];
			points[point_count].x = x = p[0];
			points[point_count].y = y = p[1];

			//LOG_MESSAGE("moveto %fx%f", points[point_count].x, points[point_count].y);
			point_count++;
			ind += 3;
			break;
		case rbgabuffer_LINETO:
			p = &ctx->commands.array[ind+1];
			points[point_count].x =p[0];
			points[point_count].y =p[1];

			//LOG_MESSAGE("lineto %fx%f", points[point_count].x, points[point_count].y);
			point_count++;
			ind += 3;
			break;
		case rbgabuffer_CLOSE:
			points[point_count].x = x;
			points[point_count].y = y;

			//LOG_MESSAGE("CLOSE %fx%f", points[point_count].x, points[point_count].y);
			//point_count++;
			ind++;
			break;
		default:
			ind++;
		}
	}

	elix_v2 y_limits, x_limits; // TODO
	rgbabuffer_edge_type edges[64]; // TODO - dynamic size array should be used.
	rgbabuffer_edge_type globaledges[64]; // TODO - dynamic size array should be used.
	rgbabuffer_active_edge_index activeedges[64] = {{UINT32_MAX, 0.0}};
	uint32_t q = 0, parity = 1;

	for (uint32_t c = 0; c < point_count; c++) {
		uint32_t n = c == point_count-1 ? 0 :c + 1;
		if (points[n].y > points[c].y ) {
			edges[c].ymin = points[c].y; //ymin
			edges[c].ymax = points[n].y; //ymax
			edges[c].xval = points[c].x; //xval
		} else {
			edges[c].ymin = points[n].y; //ymin
			edges[c].ymax = points[c].y; //ymax
			edges[c].xval = points[n].x; //xval
		}

		edges[c].slope = (points[n].x - points[c].x) / (points[n].y - points[c].y); // 1/m

		if ( c == 0 ) {
			y_limits.x = edges[0].ymin;
			y_limits.y = edges[0].ymax;
			x_limits.x = edges[0].xval;
			x_limits.y = edges[0].xval;
		}
		if ( isfinite(edges[c].slope) ) {
			if ( y_limits.x > edges[c].ymin ) { y_limits.x = edges[c].ymin; }
			if ( y_limits.y < edges[c].ymax ) { y_limits.y = edges[c].ymax; }

			if ( x_limits.x > points[c].x ) { x_limits.x = points[c].x; }
			if ( x_limits.y < points[c].x ) { x_limits.y = points[c].x; }
			if ( x_limits.y < points[n].x ) { x_limits.y = points[n].x; }

			rgbabuffer_edge_type_insert(globaledges, edges[c], q++, 64);
		}
	}
//	//List Global Edges
//	for ( uint32_t p = 0; p < point_count; p++ ) {
//		LOG_MESSAGE("%d: ymin: %f ymax: %f xval: %f slope: %f", p, globaledges[p].ymin, globaledges[p].ymax, globaledges[p].xval, globaledges[p].slope);
//	}

//	printf("     %.0f%*c%.0f\n", x_limits.x, (int)(x_limits.y-x_limits.x)-1, ' ', x_limits.y  );
	for (float line = y_limits.x; line < y_limits.y; line += 1.0f) {
		uint32_t line32 = (uint32_t)line;
		uint32_t col32;
		uint32_t i = 0;
		uint32_t max_active = 64;
		q = 0;

		// Save Global Edges for
		for (uint32_t c = 0; c < point_count; c++) {
			if ( (uint32_t)globaledges[c].ymin == line32 && globaledges[c].ymin < globaledges[c].ymax) {
				insertSorted(activeedges, { c, globaledges[c].xval }, q++, 64);
			}
		}
		activeedges[q] = { UINT32_MAX, 0.0};
		max_active = q;

		q = activeedges[0].index;

		for (float col = x_limits.x; col <= x_limits.y; col += 1.0f) {

			col32 = (uint32_t)col;

			if ( q < 64 ) {
				uint32_t next = (uint32_t)globaledges[q].xval;
				if ( next == col32 ) {
					parity = !parity;
					q = activeedges[++i].index;
				}
			}

			if (!parity)
				rbgabuffer__pixel(ctx, ctx->fill_colour, col32, line32 );

			//printf("%c", parity ? ' ' : '+' );

			//Note: Just incase of a single pixel being drawn.
			if ( q < 64 ) {
				uint32_t next = (uint32_t)globaledges[q].xval;
				if ( next == col32 ) {
					parity = !parity;
					q = activeedges[++i].index;
				}
			}
		}
//		printf("\n");

		// Update x
		for (uint32_t c = 0; c < max_active; c++) {
			uint32_t p = activeedges[c].index;
			if ( p != UINT32_MAX ) {
				globaledges[p].ymin += 1.0f;
				globaledges[p].xval += globaledges[p].slope;
			}
		}
	}
	ctx->commands.index = 0;
}

void rbgabuffer_Stroke(rbgabuffer_context* ctx) {

	float x, y;
	float * p = nullptr;
	elix_v2 points[64];
	uint32_t point_count = 0;
	size_t ind = 0;
	while (ind < ctx->commands.index) {
		uint32_t cmd = (uint32_t)ctx->commands.array[ind];
		switch (cmd) {
			case rbgabuffer_MOVETO:
				p = &ctx->commands.array[ind+1];
				points[point_count].x = x = p[0];
				points[point_count].y = y = p[1];

				//LOG_MESSAGE("moveto %fx%f", points[point_count].x, points[point_count].y);
				point_count++;

				ind += 3;
				break;
			case rbgabuffer_LINETO:
				p = &ctx->commands.array[ind+1];

				points[point_count].x =p[0];
				points[point_count].y =p[1];
				//LOG_MESSAGE("lineto %fx%f", points[point_count].x, points[point_count].y);
				point_count++;

				ind += 3;
				break;
			case rbgabuffer_CLOSE:
				points[point_count].x = x;
				points[point_count].y = y;
				//LOG_MESSAGE("CLOSE %fx%f", points[point_count].x, points[point_count].y);
				point_count++;
				ind++;
				break;
			default:
				ind++;
		}
	}
	uint32_t c = 1;
	for ( ; c < point_count; c++) {
		rbgabuffer__line(points[c-1], points[c], ctx, ctx->stroke_colour);
	}
	ctx->commands.index = 0;
}

void rbgabuffer_FillColor(rbgabuffer_context* ctx, uint32_t color) {
	ctx->fill_colour = color;
}

void rbgabuffer_StrokeColor(rbgabuffer_context* ctx, uint32_t color) {
	ctx->stroke_colour = color;
}

void rbgabuffer_MoveTo(rbgabuffer_context* ctx, float x, float y) {
	float vals[] = {
		rbgabuffer_MOVETO, x,y,
	};
	rbgabuffer__appendCommands(ctx, vals, ARRAYCOUNT(vals));

}
void rbgabuffer_LineTo(rbgabuffer_context* ctx, float x, float y) {
	float vals[] = {
		rbgabuffer_LINETO, x,y,
	};
	rbgabuffer__appendCommands(ctx, vals, ARRAYCOUNT(vals));
}



#include "elix_os.hpp"


rgbabuffer_font * rbgabuffer__unloadFont(rbgabuffer_context* ctx, rgbabuffer_font *& font) {
	elix_databuffer_free(&font->data);
	NULLIFY(font);
	return font;
}

rgbabuffer_font * rbgabuffer__loadFont(rbgabuffer_context* ctx, const char * font_name) {
	rgbabuffer_font * font = nullptr;
	elix_databuffer raw_file;

	raw_file = elix_os_font(font_name);
	if ( raw_file.size ) {
		font = new rgbabuffer_font();
		font->data = raw_file;
		int index = stbtt_GetFontOffsetForIndex(raw_file.data, 0);
		if ( index != -1 ) {
			stbtt_InitFont(&font->info, raw_file.data, index);
			stbtt_GetFontVMetrics(&font->info, &font->base_ascent, &font->base_descent, &font->base_line_gap);
	}
	}

	//elix_databuffer_free(&raw_file);

	return font;
}



void rgbabuffer__fillChar(rbgabuffer_context* ctx, rgbabuffer_font * font, uint32_t character, float &x, float &y,uint32_t next_character) {
	if ( !font || !font->info.numGlyphs) {
		return;
	}
	int index = stbtt_FindGlyphIndex(&font->info, character);
	if (character > 127 && !index) {
		printf("non-ascii %d %d\n", character, index);
	}

	if ( index ) {
		float scale = stbtt_ScaleForPixelHeight(&font->info, ctx->font_size_px);
		float baseline = (font->base_ascent * scale);
		int advance_width, left_sidebearing,width, height, xoff, yoff;

		uint8_t * bitmap = stbtt_GetGlyphBitmap(&font->info, 0, scale, index, &width, &height, &xoff, &yoff);

		stbtt_GetGlyphHMetrics(&font->info, character, &advance_width, &left_sidebearing);

		for ( int32_t j = 0; j < height; ++j) {
			for ( int32_t i = 0; i < width; ++i) {
				float alpha = (float)bitmap[(j*width)+i] / 255.0;
				rbgabuffer__pixel(ctx, 0xFF000000, x+i, y+j+yoff+baseline, alpha);
			}
		}

		x += (float)advance_width * scale;
		if ( next_character ) {
			x += scale * stbtt_GetCodepointKernAdvance(&font->info, character, next_character);
		}
		delete bitmap;
	} else if ( ctx->emoji ) {
		rgbabuffer__fillChar( ctx, ctx->emoji, character, x, y, next_character);
	}
}

#include "elix_cstring.hpp"

void rbgabuffer_FillText(rbgabuffer_context* ctx, const char * text, float x, float y, float maxWidth) {
	rgbabuffer_font * font = rbgabuffer__loadFont(ctx, "Sans-Serif");
	if ( !font ) {
		return;
	}

	char * object = (char*)text;
	uint32_t current_character = 0, next_character = 0;
	while ( (current_character = elix_cstring_next_character(object)) > 0  ) {
		next_character = elix_cstring_peek_character(object);
		rgbabuffer__fillChar(ctx, font, current_character,x, y, next_character);

	}
	delete font;
}

elix_text_metrics rbgabuffer_measureText(rbgabuffer_context* ctx, const char * text) {
	elix_text_metrics metrics = {0.0f};
	rgbabuffer_font  * font = rbgabuffer__loadFont(ctx, ctx->font_family);
	float font_scale = stbtt_ScaleForPixelHeight(&font->info, ctx->font_size_px);
	int ascent, descent;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, nullptr);
	int baseline = (int) (ascent * font_scale);
	int descent_scaled = (int) (descent * font_scale);

	char * object = (char*)text;
	uint32_t current_character = 0, next_character = 0;
	while ( (current_character = elix_cstring_next_character(object)) > 0  ) {
		next_character = elix_cstring_peek_character(object);

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

rbgabuffer_context * rbgabuffer_create_context(elix_graphic_data * external_buffer, const elix_uv32_2 requested_dimensions ) {
	rbgabuffer_context * context = new rbgabuffer_context();

	context->internal_buffer = (external_buffer == nullptr);

	if (context->internal_buffer) {
		ASSERT(requested_dimensions.width != 0);
		ASSERT(requested_dimensions.height != 0);
		context->memory = elix_graphic_data_create(requested_dimensions);

		rbgabuffer__testpattern(context->memory);
	} else {
		ASSERT(external_buffer->bpp == 4);
		context->memory = external_buffer;

		rbgabuffer__gridpattern(context->memory);
	}
	context->dimensions = requested_dimensions;
	context->commands.max = 32;
	context->commands.array = (float*)malloc( sizeof(float)*context->commands.max);

	context->emoji = rbgabuffer__loadFont(context, nullptr);

	return context;
}

rbgabuffer_context * rgbabuffer_delete_context(rbgabuffer_context *& ctx) {
	ASSERT(ctx);
	free(ctx->commands.array);
	if (ctx->internal_buffer) {
		ctx->memory = elix_graphic_data_destroy(ctx->memory);
	}
	NULLIFY(ctx);
	return ctx;
}

