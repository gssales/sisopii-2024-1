#include "include/discovery.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace discovery;

void *discovery::service(Station *station)
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
			proc_host(station);
	}
	return 0;
}

/**
 * Client Logic
*/
void discovery::proc_host(Station *station)
{
	if (station->GetManager() == NULL)
	{
		auto discovery_request = network::create_packet(network::DISCOVERY_REQUEST, station->serialize());
		auto response = network::datagram(INADDR_BROADCAST, discovery_request);
		if (response.type == network::DISCOVERY_RESPONSE && response.status == network::SUCCESS)
		{
			std::cout << "New Manager Found!" << std::endl;
			Station manager;
			Station::deserialize(&manager, response.station);
			station->SetManager(&manager);
			station->GetManager()->print();
		}
	}
	std::this_thread::sleep_for(std::chrono::seconds(10));
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
			std::cout << "New Host Found!" << std::endl;

			auto response = network::create_packet(network::DISCOVERY_RESPONSE, station->serialize(), station->GetClock());
			response.status = network::SUCCESS;
			resolve(response);
		}
	}

	if (station->GetType() == HOST)
	{
		if (data.type == network::LEAVING)
		{
			station->SetManager(NULL);
		}
	}
	
	return 0;
}
