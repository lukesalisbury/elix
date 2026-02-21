#ifndef ELIX_PIXELS_HPP
#define ELIX_PIXELS_HPP

#include "elix_core.h"

enum elix_pixels_modes {
	EPM_NONE   = 0,
	EPM_C8     = 0 << 1,
	EPM_RGB565 = 0 << 2,
	EPM_XRGB8  = 0 << 3,
	EPM_ARGB8  = 0 << 4,

};

#endif // ELIX_PIXELS_HPP
