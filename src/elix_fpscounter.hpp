/***********************************************************************************************************************
 Copyright (c) Luke Salisbury
 This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
 liable for any damages arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
 it and redistribute it freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
	If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is
	not required.
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
	software.
 3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/

#ifndef ELIX_FPSCOUNTER_HPP
#define ELIX_FPSCOUNTER_HPP

#include <chrono>
#include <stdio.h>

class elix_fpscounter {
	public:
	char text[8] = {0};
	uint32_t counter = 0;
	std::chrono::duration<double> delta;
	std::chrono::time_point<std::chrono::high_resolution_clock> last;
	void start() {
		last = std::chrono::high_resolution_clock::now();
		delta = std::chrono::milliseconds(0);
	}
	void update() {
		counter++;
		delta = std::chrono::high_resolution_clock::now() - last;
		if (delta >= std::chrono::milliseconds(1000) ) {
			snprintf(text, 6, "%u", counter);
			last = std::chrono::high_resolution_clock::now();
			delta = std::chrono::milliseconds(0);
			counter = 0;
		}
	}
};

#endif // ELIX_FPSCOUNTER_HPP
