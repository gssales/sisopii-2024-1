#include "include/monitoring.h"

#include <iostream>
#include <thread>
#include "include/utils.h"

using namespace monitoring;

void *monitoring::service(service_params_t *params)
{
  auto station = params->station;

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
      break;
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

  uint64_t refresh = get_option(options, OPT_REFRESH, 10) * 1000; // default 10 seconds
  int max_retry = get_option(options, OPT_RETRY, 2); // default 2 retries
  for (auto &host_pair : station_table->clone())
  {
    auto hostname = host_pair.first;
    auto host = host_pair.second.first;
    auto host_info = host_pair.second.second;

    if (host.macAddress == station->GetMacAddress())
      continue;

    if (now() - host_info.last_update < refresh)
    {
      logger->info("Monitoring " + hostname);
      auto monitoring_request =  network::create_packet(network::MONITORING_REQUEST, station->serialize());
      auto response = network::packet(inet_addr(host.ipAddress), monitoring_request, logger, options);
      if (response.status == network::SUCCESS)
      {
        logger->info("Awaken");
        station_table->update_retry(hostname, 0);
        station_table->update(hostname, AWAKEN, host.type);
        params->ui_lock.unlock();
      } 
      else if (host.status != ASLEEP) 
      {
        logger->info("Failed to request " + hostname + " retrying " + std::to_string(host_info.retry_counter + 1) + "/" + std::to_string(max_retry));
        station_table->update_retry(hostname, host_info.retry_counter + 1);
        if (host_info.retry_counter + 1 >= max_retry)
          station_table->update(hostname, ASLEEP, host.type);
        params->ui_lock.unlock();
      }
    }
  }
  int sleep = get_option(options, OPT_SLEEP, 5);
	std::this_thread::sleep_for(std::chrono::seconds(sleep));
}

void *monitoring::process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve) 
{
  auto station = params->station;

	if (station->GetType() == HOST)
	{
		if (data.type == network::MONITORING_REQUEST)
    {
      auto response = network::create_packet(network::MONITORING_RESPONSE, station->serialize(), station->GetClock());
      response.status = network::SUCCESS;
      resolve(response);
    }
	}
	
	return 0;
}