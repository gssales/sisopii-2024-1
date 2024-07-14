#include <iostream>
#include <cstring>
#include <thread>
#include <map>
#include <string>

#include "include/station.h"
#include "include/network.h"
#include "include/discovery.h"
#include "include/monitoring.h"
#include "include/interface.h"
#include "include/utils.h"
#include "include/options_parser.h"

int main(int argc, const char *argv[]) {
	auto options = option_t();
	options[OPT_PORT_DGRAM] = 50505;
	options[OPT_PORT_STREAM] = 50506;
	options[OPT_TIMEOUT] = 1;
	options[OPT_SLEEP] = 5;
	options[OPT_REFRESH] = 10;
	options[OPT_RETRY] = 2;
	options[OPT_DEBUG] = 0;
	parseOptions(argc, argv, &options);

	auto station = new Station();
	station->init();
	if (get_option(&options, OPT_MANAGER, 0) == 1)
		station->SetType(MANAGER);
	// station->print();

	station->SetStationTable(new StationTable());
	if (station->GetType() == MANAGER)
	{
		station->GetStationTable()->insert(station->GetHostname(), station->serialize());
	}

	auto udp_thread = std::thread(&network::udp_server, &options, station);
	auto tcp_thread = std::thread(&network::tcp_server, &options, station);
	auto discovery_thread = std::thread(&discovery::service, &options, station);
	auto monitoring_thread = std::thread(&monitoring::service, &options, station);
	auto interface_thread = std::thread(&interface::interface, &options, station);

	udp_thread.join();
	tcp_thread.join();
	discovery_thread.join();
	monitoring_thread.join();
	interface_thread.join();

	return 0;
}