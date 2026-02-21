#ifndef ELIX_RGBAIMAGE_CANVAS_HEADER
#define ELIX_RGBAIMAGE_CANVAS_HEADER

#include "elix_core.h"
#include "elix_pixels.h"
#include "elix_graphics.h"
#include "stb_truetype.h"

/*

namespace elix {
	namespace rgbaimage {
		namespace canvas {

		void save();
		void restore();

		void scale( float x, float y);
		void rotate( float angle);
		void translate(float  x, float y);
		void transform(a, b, c, d, e, f);
		void setTransform (a, b, c, d, e, f);

		void strokeStyle
		void fillStyle
		void createLinearGradient (x0, y0, x1, y1) and addColorStop (pos, color)
		void createRadialGradient (x0, y0, r0, x1, y1, r1) and addColorStop (pos, color)
		void createPattern (image, repetition)
		2.2.6 Line caps and joins
		2.2.6.1 lineWidth
		2.2.6.2 lineCap
		2.2.6.3 lineJoin
		2.2.6.4 miterLimit
		2.2.7.1 shadowOffsetX
		2.2.7.2 shadowOffsetY
		2.2.7.3 shadowBlur
		2.2.7.4 shadowColor

		2.2.8.1 fillRect (x, y, w, h)
		2.2.8.2 strokeRect (x, y, w, h)
		2.2.8.3 clearRect (x, y, w, h)

		2.2.9.1 beginPath()
		2.2.9.2 closePath()
		2.2.9.3 stroke()
		2.2.9.4 fill()
		2.2.9.5 lineTo (x, y)
		2.2.9.6 moveTo (x, y)
		2.2.9.7 rect (x, y, w, h)
		2.2.9.8 quadraticCurveTo (cpx, cpy, x, y)
		2.2.9.9 bezierCurveTo (cp1x, cp1y, cp2x, cp2y, x, y)
		2.2.9.10 arc (x, y, r, start, end, anticlockwise)
		2.2.9.11 arcTo (x1, y1, x2, y2, radius)
		2.2.9.12 clip()
		2.2.9.13 isPointInPath (x, y)

		2.2.10.1 font
		2.2.10.2 textAlign
		2.2.10.3 textBaseline
		2.2.10.4 fillText (text, x, y) and fillText (text, x, y, maxWidth)
		2.2.10.5 strokeText (text, x, y) and strokeText (text, x, y, maxWidth)
		2.2.10.6 measureText(text).width

		2.2.11.1 drawImage (image, dx, dy)
		2.2.11.2 drawImage (image, dx, dy, dw, dy)
		2.2.11.3 drawImage (image, sx, sy, sw, sh, dx, dy, dw, dh)

	}
}
		*/

#define rbgabuffer_DEFAULT_FONTSIZE 16.0f
#define rbgabuffer_DEFAULT_FONTFAMILY "Sans-Serif"


// Make rbgabuffer_commands private
enum rbgabuffer_commands {
	rbgabuffer_MOVETO = 0,
	rbgabuffer_LINETO = 1,
	rbgabuffer_BEZIERTO = 2,
	rbgabuffer_CLOSE = 3,
	rbgabuffer_WINDING = 4,
};

typedef struct rbgabuffer_command_array {
	float * array;
	uint32_t index;
	uint32_t max;
} rbgabuffer_command_array;

typedef struct rgbabuffer_font {
	stbtt_fontinfo info;
	elix_databuffer data;
	float scale, baseline;
	int base_ascent, base_descent, base_line_gap;
} rgbabuffer_font;


typedef struct rbgabuffer_colour_stop {
	float pos;
	uint32_t col;
} rbgabuffer_colour_stop;

typedef struct rbgabuffer_linear_gradient {
	uint32_t count;
	rbgabuffer_colour_stop stops[16]; // TODO
} rbgabuffer_linear_gradient;

typedef struct rbgabuffer_radial_gradient {
	uint32_t count;
	rbgabuffer_colour_stop stops[16]; // TODO
} rbgabuffer_radial_gradient;

typedef struct rbgabuffer_pattern {
	elix_graphic_data * image;
	uint32_t repeat_mode;
} rbgabuffer_pattern;

typedef struct rbgabuffer_context {
	elix_graphic_data * memory;
	uint8_t internal_buffer; //Note: Owned by this
	elix_uv32_2 dimensions;


	rgbabuffer_font * emoji;
	rbgabuffer_command_array commands;

	uint32_t fill_colour;
	uint32_t stroke_colour;

	char font_family[16];
	float font_size_px;
} rbgabuffer_context;

typedef struct elix_text_metrics {
	double width;
} elix_text_metrics;


#ifdef __cplusplus
extern "C" {
#endif

rbgabuffer_context * rbgabuffer_create_context(elix_graphic_data * external_buffer, const elix_uv32_2 requested_dimensions );
rbgabuffer_context * rgbabuffer_delete_context(rbgabuffer_context * ctx);
void rbgabuffer_testpattern(rbgabuffer_context *ctx);
uint32_t rbgabuffer_get_pixel(rbgabuffer_context * ctx, uint32_t x, uint32_t y);

void rbgabuffer_BeginPath(rbgabuffer_context* ctx);
void rbgabuffer_MoveTo(rbgabuffer_context* ctx, float x, float y);
void rbgabuffer_LineTo(rbgabuffer_context* ctx, float x, float y);
void rbgabuffer_BezierTo(rbgabuffer_context* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);
void rbgabuffer_QuadTo(rbgabuffer_context* ctx, float cx, float cy, float x, float y);
void rbgabuffer_ArcTo(rbgabuffer_context* ctx, float x1, float y1, float x2, float y2, float radius);
void rbgabuffer_ClosePath(rbgabuffer_context* ctx);
void rbgabuffer_PathWinding(rbgabuffer_context* ctx, int dir);
void rbgabuffer_Arc(rbgabuffer_context* ctx, float cx, float cy, float r, float a0, float a1, int dir);
void rbgabuffer_Rect(rbgabuffer_context* ctx, float x, float y, float w, float h);
void rbgabuffer_RoundedRect(rbgabuffer_context* ctx, float x, float y, float w, float h, float r);
void rbgabuffer_RoundedRectVarying(rbgabuffer_context* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
void rbgabuffer_Ellipse(rbgabuffer_context* ctx, float cx, float cy, float rx, float ry);
void rbgabuffer_Circle(rbgabuffer_context* ctx, float cx, float cy, float r);


void rbgabuffer_Fill(rbgabuffer_context* ctx);
void rbgabuffer_Stroke(rbgabuffer_context* ctx);

void rbgabuffer_StrokeColor(rbgabuffer_context* ctx, uint32_t color );
void rbgabuffer_FillColor(rbgabuffer_context* ctx, uint32_t color);


void rbgabuffer_FillText(rbgabuffer_context* ctx, const char * text, float x, float y, float maxWidth);
elix_text_metrics rbgabuffer_measureText(rbgabuffer_context* ctx, const char * text);


#ifdef __cplusplus
}
#endif


#endif // ELIX_RGBAIMAGE_CANVAS_HEADER
