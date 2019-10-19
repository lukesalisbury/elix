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
