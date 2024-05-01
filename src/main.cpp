#include <iostream>
#include <cstring>
#include <thread>

#include "include/station.h"
#include "include/network.h"
#include "include/discovery.h"
#include "include/utils.h"

int main(int argc, const char *argv[]) {

	auto station = new Station();

	station->init();
	if (argc > 1 && strcmp(argv[1], "manager") == 0)
		station->SetType(MANAGER);
	station->print();

	auto udp_thread = std::thread(&network::udp_server, station);
	auto discovery_thread = std::thread(&discovery::service, station);

	udp_thread.join();
	discovery_thread.join();

	return 0;
}
