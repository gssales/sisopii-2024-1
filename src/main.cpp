#include <iostream>
#include <cstring>
#include <thread>
#include <map>
#include <string>

#include "include/station.h"
#include "include/network.h"
#include "include/discovery.h"
#include "include/monitoring.h"
#include "include/election.h"
#include "include/interface.h"
#include "include/utils.h"
#include "include/options_parser.h"
#include "include/logger.h"
#include "include/service.h"
#include <signal.h>

service_params_t params;

void handle_interrupt(int signal)
{
	if (signal == 2) {
		discovery::leave(&params);
	}
}

int main(int argc, const char *argv[]) {
	auto options = options_t();
	options[OPT_PORT_DGRAM] = UDP_PORT;
	options[OPT_PORT_STREAM] = TCP_PORT;
	options[OPT_TIMEOUT] = 1;
	options[OPT_SLEEP] = 5;
	options[OPT_REFRESH] = 10;
	options[OPT_RETRY] = 2;
	options[OPT_DEBUG] = 0;
	options[OPT_GENERATE_PID] = 0;
	parseOptions(argc, argv, &options);

	params.options = &options;

	auto station = new Station();
	station->init();
	if (get_option(&options, OPT_MANAGER, 0) == 1)
		station->SetType(MANAGER);
	if (get_option(&options, OPT_GENERATE_PID, 0) == 1)
		station->GeneratePid();
	params.station = station;

	auto station_table = new StationTable();
	if (station->GetType() == MANAGER)
		station_table->insert(station->GetHostname(), station->serialize());
	params.station_table = station_table;

	auto logger = new Logger(&params.ui_lock);
	logger->debug = get_option(&options, OPT_DEBUG, 0) == 1;
	logger->info("Starting station " + station->GetHostname());
	params.logger = logger;

	auto udp_thread = std::thread(&network::udp_server, &params);
	auto tcp_thread = std::thread(&network::tcp_server, &params);
	auto discovery_thread = std::thread(&discovery::service,&params);
	auto monitoring_thread = std::thread(&monitoring::service,&params);
	auto election_thread = std::thread(&election::service,&params);
	auto interface_thread = std::thread(&interface::interface,&params);
	auto command_thread = std::thread(&interface::command,&params);

	signal(SIGINT, handle_interrupt);

	udp_thread.join();
	tcp_thread.join();
	discovery_thread.join();
	monitoring_thread.join();
	election_thread.join();
	interface_thread.join();
	command_thread.join();

	return 0;
}