#include "include/monitoring.h"

#include <iostream>
#include <thread>
#include "include/replication.h"
#include "include/utils.h"

using namespace monitoring;

void *monitoring::service(service_params_t *params)
{
  auto station = params->station;
	int sleep = get_option(params->options, OPT_SLEEP, 5);
  while (station->GetStatus() != EXITING)
  {
    /**
     * Na estação manager, o serviço de monitoramento é ativo
     * itera sobre os hosts, atualizando sleep status
     */
    if (station->GetType() == MANAGER)
      proc_manager(params);
      
    /**
     * Na estação participante, o serviço de monitoramento é passivo
     * Termina thread
     */
    if (station->GetType() == HOST)
			std::this_thread::sleep_for(std::chrono::seconds(sleep));
  }
  return 0;
}

/**
 * Manager Logic
*/
void monitoring::proc_manager(service_params_t *params)
{
  auto options = params->options;
  auto station = params->station;
  auto station_table = params->station_table;
  auto logger = params->logger;

  if (station_table->table.size() > 0)
  {
    uint64_t refresh = get_option(options, OPT_REFRESH, 10) * 1000; // default 10 seconds
    int max_retry = get_option(options, OPT_RETRY, 2); // default 2 retries
    
    std::vector<station_serial> stations;
    for (auto &host_pair : station_table->clone())
    {
      auto host = host_pair.second.first;
      auto host_info = host_pair.second.second;
      if (host.macAddress == station->GetMacAddress())
        continue;

      if (now() - host_info.last_update >= refresh)
        stations.push_back(host);
    }

    if (stations.size() > 0)
    {
      auto monitoring_request =  network::create_packet(network::MONITORING_REQUEST, station->serialize());
      auto future_responses = network::multicast(stations, monitoring_request, logger, params->options);
      auto responses = future_responses.get();
      for (auto &response : responses)
      {	
        auto packet = response.second;
        auto hostname = response.first.hostname;
        auto host = station_table->table[hostname].first;
        if (packet.status == network::SUCCESS)
        {
          station_table->update_retry(hostname, true);
          station_table->update(hostname, AWAKEN, host.type);
          replication::replicate(params, "Host awaken");
          params->ui_lock.unlock();
        } 
        else if (host.status != ASLEEP) 
        {
          uint8_t retry_counter = station_table->update_retry(hostname, false);
          if (retry_counter >= max_retry)
            station_table->update(hostname, ASLEEP, host.type);
          replication::replicate(params, "Host asleep");
          params->ui_lock.unlock();
        }
      }
    }
  }

  int sleep = get_option(options, OPT_SLEEP, 5);
	std::this_thread::sleep_for(std::chrono::seconds(sleep));
}

void *monitoring::process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close)
{
  auto station = params->station;

	if (station->GetType() == HOST)
	{
		if (data.type == network::MONITORING_REQUEST)
    {
      auto response = network::create_packet(network::MONITORING_RESPONSE, station->serialize(), station->GetClock());
      response.status = network::SUCCESS;
      resolve(response);
			return 0;
    }
	}
	
	close();
	return 0;
}