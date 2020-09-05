#ifndef ELIX_V_H
#define ELIX_V_H

#include "elix_core.h"

typedef float real32;
typedef double real64;
typedef real32 real;

union v2 {
	struct {
		real x, y;
	};
	struct {
		real u, v;
	};
	struct {
		real w, h;
	};
	real E[2];
};

union v3 {
	struct {
		real x, y, z;
	};
	struct {
		real u, v, __;
	};
	struct {
		real r, g, b;
	};
	struct {
		v2 xy;
		real _0;
	};
	struct {
		real _1;
		v2 yz;
	};
	struct {
		v2 uv;
		real _2;
	};
	real E[3];
};

union v4 {
	struct {
		union {
			v3 xyz;
			struct {
				real x, y, z;
			};
		};
		real w;
	};
	struct {
		union {
			v3 rgb;
			struct {
				real r, g, b;
			};
		};
		real a;
	};
	struct {
		v2 xy;
		real _0;
		real _1;
	};
	struct {
		real _2;
		v2 yz;
		real _3;
	};
	struct {
		real _4;
		real _5;
		v2 zw;
	};
	struct {
		real ox;
		real oy;
		real ow;
		real oh;
	};
	real E[4];
};

union v5 {
	struct {
		real x, y, z, u, v;
	};
	real E[5];
};

union mat4 {
	real m[4][4];
	struct {
		real m00, m01, m02, m03;
		real m10, m11, m12, m13;
		real m20, m21, m22, m23;
		real m30, m31, m32, m33;
	};
	real a[16];
} ;

inline v3 operator+(v3 a, v3 b) {
	v3 result;

	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;

	return(result);
}

#endif // ELIX_V_H
