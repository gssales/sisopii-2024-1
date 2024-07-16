#include "include/discovery.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace discovery;

void *discovery::service(service_params_t *params)
{
	auto station = params->station;
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
			proc_host(params);
	}
	return 0;
}

/**
 * Client Logic
*/
void discovery::proc_host(service_params_t *params)
{
	auto options = params->options;
	auto station = params->station;
	auto logger = params->logger;

	if (station->GetManager() == NULL)
	{
		auto discovery_request = network::create_packet(network::DISCOVERY_REQUEST, station->serialize());
		auto response = network::datagram(INADDR_BROADCAST, discovery_request, logger, options);
		if (response.type == network::DISCOVERY_RESPONSE && response.status == network::SUCCESS)
		{
			Station manager;
			Station::deserialize(&manager, response.station);
			station->SetManager(&manager);
			params->ui_lock.unlock();
		}
	}
	int sleep = get_option(options, OPT_SLEEP, 5);
	std::this_thread::sleep_for(std::chrono::seconds(sleep));
}
	
/**
 * Server Logic
*/
void *discovery::process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve)
{
	auto station = params->station;
	auto station_table = params->station_table;

	if (station->GetType() == MANAGER)
	{
		if (data.type == network::DISCOVERY_REQUEST)
		{
			station_table->insert(data.station.hostname, data.station);
			params->ui_lock.unlock();

			auto response = network::create_packet(network::DISCOVERY_RESPONSE, station->serialize(), station->GetClock());
			response.status = network::SUCCESS;
			resolve(response);
		}
		else if (data.type == network::LEAVING)
		{	
			station_table->remove(data.station.hostname);
			params->ui_lock.unlock();
		}
	}

	if (station->GetType() == HOST)
	{
		if (data.type == network::LEAVING && station->GetManager()->GetMacAddress().compare(data.station.macAddress) == 0)
		{
			station->SetManager(NULL);
			params->ui_lock.unlock();
		}
	}
	
	return 0;
}

void discovery::leave(service_params_t *params)
{
  params->station->SetStatus(EXITING);
  auto leaving_message = network::create_packet(network::LEAVING, params->station->serialize());
  network::datagram(INADDR_BROADCAST, leaving_message, params->logger, params->options);
  params->ui_lock.unlock();
}

