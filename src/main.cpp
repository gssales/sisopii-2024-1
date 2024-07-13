#include <iostream>
#include <cstring>
#include <thread>

#include "include/station.h"
#include "include/network.h"
#include "include/discovery.h"
#include "include/monitoring.h"
#include "include/interface.h"
#include "include/utils.h"

int main(int argc, const char *argv[]) {

	auto station = new Station();

	station->init();
	if (argc > 1 && strcmp(argv[1], "manager") == 0)
		station->SetType(MANAGER);

	if (station->GetType() == MANAGER)
	{
		station->SetStationTable(new StationTable());
		station->GetStationTable()->insert(station->GetHostname(), station->serialize());
	}

	auto udp_thread = std::thread(&network::udp_server, station);
	auto tcp_thread = std::thread(&network::tcp_server, station);
	auto discovery_thread = std::thread(&discovery::service, station);
	auto monitoring_thread = std::thread(&monitoring::service, station);
	auto interface_thread = std::thread(&interface::screen, station);

	udp_thread.join();
	tcp_thread.join();
	discovery_thread.join();
	monitoring_thread.join();
	interface_thread.join();

	return 0;
}