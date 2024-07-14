#include "include/discovery.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace discovery;

void *discovery::service(option_t *options, Station *station)
{
	while (station->GetStatus() != EXITING)
	{
		/*
		 * Na estação manager, o serviço de discory é passivo
		 * Termina thread
		 */
		if (station->GetType() == MANAGER)
			break;
		
		/*
		 * Na estação participante, o serviço de discory é ativo
		 * Busca manager
		 */
		if (station->GetType() == HOST)
			proc_host(options, station);
	}
	return 0;
}

/**
 * Client Logic
*/
void discovery::proc_host(option_t *options, Station *station)
{
	int timeout = get_option(options, OPT_TIMEOUT, 1);
	int port = get_option(options, OPT_PORT_DGRAM, UDP_PORT);
	if (station->GetManager() == NULL)
	{
		auto discovery_request = network::create_packet(network::DISCOVERY_REQUEST, station->serialize());
		auto response = network::datagram(INADDR_BROADCAST, discovery_request, timeout, port);
		if (response.type == network::DISCOVERY_RESPONSE && response.status == network::SUCCESS)
		{
			Station manager;
			Station::deserialize(&manager, response.station);
			station->SetManager(&manager);
		}
	}
	int sleep = get_option(options, OPT_SLEEP, 5);
	std::this_thread::sleep_for(std::chrono::seconds(sleep));
}
	
/**
 * Server Logic
*/
void *discovery::process_request(Station *station, packet_t data, std::function<void(packet_t)> resolve)
{
	if (station->GetType() == MANAGER)
	{
		if (data.type == network::DISCOVERY_REQUEST)
		{
			station->GetStationTable()->insert(data.station.hostname, data.station);

			auto response = network::create_packet(network::DISCOVERY_RESPONSE, station->serialize(), station->GetClock());
			response.status = network::SUCCESS;
			resolve(response);
		}
	}

	if (station->GetType() == HOST)
	{
		if (data.type == network::LEAVING && data.station.macAddress == station->GetManager()->GetMacAddress())
		{
			station->SetManager(NULL);
		}
	}
	
	return 0;
}
