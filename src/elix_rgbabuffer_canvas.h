#ifndef ELIX_RGBABUFFER_CANVAS_HPP
#define ELIX_RGBABUFFER_CANVAS_HPP

#include "elix_rgbabuffer.hpp"
// Simple Class
class CanvasContext {
	private:

	protected:

	public:
		void save();
		void restore();

		void scale( float x, float y);
		void rotate( float angle);
		void translate( float x, float y);
		void transform(float a, float b, float c, float d, float e, float f);
		void setTransform (float a, float b, float c, float d, float e, float f);

		float globalAlpha = 1.0;
		char * globalCompositeOperation;
		char * strokeStyle;
		char * fillStyle;
		rbgabuffer_linear_gradient createLinearGradient( float x0, float y0, float x1, float y1);
		rbgabuffer_radial_gradient createRadialGradient( float x0, float y0, float x1, float y1);

		rbgabuffer_pattern createPattern(elix_graphic_data * image, uint32_t repetition);
		float lineWidth;
		char * lineCap;
		char * lineJoin;
		char * miterLimit;
		float shadowOffsetX;
		float shadowOffsetY;
		float shadowBlur;
		char * shadowColor;

		void fillRect (float x, float y, float w, float h);
		void strokeRect (float x, float y, float w, float h);
		void clearRect (float x, float y, float w, float h);

		void beginPath();
		void closePath();
		void stroke();
		void fill();
		void lineTo(float x, float y);
		void moveTo(float x, float y);
		void rect(float x, float y, float w, float h);
		void quadraticCurveTo(float cpx, float cpy, float x, float y);
		void bezierCurveTo (float cp1x, float cp1y, float cp2x,float  cp2y, float x, float y);
		void arc (float x, float y, float r, float start, float end, bool anticlockwise = false);
		void arcTo (float x1, float y1, float x2, float y2, float radius);
		void clip();
		void isPointInPath (float x, float y);

		char * font;
		char * textAlign;
		char * textBaseline;
		void fillText (char * text, float x, float y, float maxWidth);
		void strokeText (char * text, float x, float y, float maxWidth);
		float measureText(char * text);

		void drawImage( elix_graphic_data * image, float dx, float dy);
		void drawImage( elix_graphic_data * image, float dx, float dy,float dw, float dh);
		void drawImage( elix_graphic_data * image, float sx, float sy,float sw, float sh, float dx, float dy,float dw, float dh);


		elix_graphic_data * getImageData(float sx, float sy, float sw, float sh);
		void putImageData(elix_graphic_data *imagedata, float dx, float dy, float sx, float sy,float sw, float sh);

};



class RGBABufferContext: CanvasContext {
	private:

	protected:

	public:
		void save();
		void restore();

		void scale( float x, float y);
		void rotate( float angle);
		void translate( float x, float y);
		void transform(float a, float b, float c, float d, float e, float f);
		void setTransform (float a, float b, float c, float d, float e, float f);

		float globalAlpha = 1.0;
		char * globalCompositeOperation;// = "source-over";
		char * strokeStyle;// = "black";
		char * fillStyle;// = "black";
		rbgabuffer_linear_gradient createLinearGradient( float x0, float y0, float x1, float y1);
		rbgabuffer_radial_gradient createRadialGradient( float x0, float y0, float x1, float y1);

		rbgabuffer_pattern createPattern(elix_graphic_data * image, uint32_t repetition);
		float lineWidth;
		char * lineCap;
		char * lineJoin;
		char * miterLimit;
		float shadowOffsetX;
		float shadowOffsetY;
		float shadowBlur;
		char * shadowColor;

		void fillRect (float x, float y, float w, float h);
		void strokeRect (float x, float y, float w, float h);
		void clearRect (float x, float y, float w, float h);

		void beginPath();
		void closePath();
		void stroke();
		void fill();
		void lineTo(float x, float y);
		void moveTo(float x, float y);
		void rect(float x, float y, float w, float h);
		void quadraticCurveTo(float cpx, float cpy, float x, float y);
		void bezierCurveTo (float cp1x, float cp1y, float cp2x,float  cp2y, float x, float y);
		void arc (float x, float y, float r, float start, float end, bool anticlockwise = false);
		void arcTo (float x1, float y1, float x2, float y2, float radius);
		void clip();
		void isPointInPath (float x, float y);

		char * font;
		char * textAlign;
		char * textBaseline;
		void fillText (char * text, float x, float y, float maxWidth);
		void strokeText (char * text, float x, float y, float maxWidth);
		float measureText(char * text);

		void drawImage( elix_graphic_data * image, float dx, float dy);
		void drawImage( elix_graphic_data * image, float dx, float dy,float dw, float dh);
		void drawImage( elix_graphic_data * image, float sx, float sy,float sw, float sh, float dx, float dy,float dw, float dh);


		elix_graphic_data * getImageData(float sx, float sy, float sw, float sh);
		void putImageData(elix_graphic_data *imagedata, float dx, float dy, float sx, float sy,float sw, float sh);

};


#endif // ELIX_RGBABUFFER_CANVAS_HPP
