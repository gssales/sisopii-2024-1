#include "include/discovery.h"

#include <iostream>
#include <thread>
#include <chrono>

#include "include/replication.h"
#include "include/election.h"

using namespace discovery;

void *discovery::service(service_params_t *params)
{
	auto station = params->station;
	int sleep = get_option(params->options, OPT_SLEEP, 5);
	while (station->GetStatus() != EXITING)
	{
		/*
		 * Na estação manager, o serviço de discory é passivo
		 * Termina thread
		 */
		if (station->GetType() == MANAGER)
			std::this_thread::sleep_for(std::chrono::seconds(sleep));
		
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
		auto response = network::datagram(INADDR_BROADCAST, discovery_request, logger, options, true);
		if (response.type == network::DISCOVERY_RESPONSE && response.status == network::SUCCESS)
		{
			auto manager = new Station();
			Station::deserialize(manager, response.station);
			station->SetManager(manager);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			params->ui_lock.unlock();
		}
		else {
			auto max_retries = get_option(options, OPT_RETRY, 2);
			station->retry_counter_manager++;
			if (station->retry_counter_manager >= max_retries)
			{
				station->retry_counter_manager = 0;
				election::start_election(params);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				params->ui_lock.unlock();
			}
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
			auto response = network::create_packet(network::DISCOVERY_RESPONSE, station->serialize(), station->GetClock());
			response.status = network::SUCCESS;
			resolve(response);

			station_table->insert(data.station.hostname, data.station);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			replication::replicate(params, "Station inserted");
			params->ui_lock.unlock();
		}
		else if (data.type == network::LEAVING)
		{	
			station_table->remove(data.station.hostname);
			replication::replicate(params, "Station removed");
			params->ui_lock.unlock();
		}
	}

	if (station->GetType() == HOST)
	{
		if (data.type == network::LEAVING && station->GetManager() != NULL && station->GetManager()->GetMacAddress().compare(data.station.macAddress) == 0)
		{
			station->SetManager(NULL);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			params->ui_lock.unlock();
		}
	}
	
	return 0;
}

void discovery::leave(service_params_t *params)
{
	if (params->station->GetType() == MANAGER)
	{
		params->station_table->remove(params->station->GetHostname());
		replication::replicate(params, "leaving");
	}
  params->station->SetStatus(EXITING);
  auto leaving_message = network::create_packet(network::LEAVING, params->station->serialize());
  network::datagram(INADDR_BROADCAST, leaving_message, params->logger, params->options, false);
  params->ui_lock.unlock();
}

