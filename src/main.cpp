#include "include/station.h"

int main(int argc, const char *argv[]) {
	auto station = new Station();

	station->init();
	station->print();

	return 0;
}
