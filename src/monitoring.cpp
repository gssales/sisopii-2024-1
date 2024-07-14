#include "include/monitoring.h"

#include <iostream>
#include <thread>
#include "include/utils.h"

using namespace monitoring;

void *monitoring::service(option_t *options, Station *station)
{
  while (station->GetStatus() != EXITING)
  {
    /**
     * Na estação manager, o serviço de monitoramento é ativo
     * itera sobre os hosts, atualizando sleep status
     */
    if (station->GetType() == MANAGER)
      proc_manager(options, station);
      
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
void monitoring::proc_manager(option_t *options, Station *station)
{
  uint64_t refresh = get_option(options, OPT_REFRESH, 10) * 1000; // default 10 seconds
  int timeout = get_option(options, OPT_TIMEOUT, 1); // default 1 second
  int max_retry = get_option(options, OPT_RETRY, 2); // default 2 retries
  int port = get_option(options, OPT_PORT_STREAM, TCP_PORT);
  for (auto &host_pair : station->GetStationTable()->clone())
  {
    auto hostname = host_pair.first;
    auto host = host_pair.second.first;
    auto host_info = host_pair.second.second;

    if (host.macAddress == station->GetMacAddress())
      continue;

    if (now() - host_info.last_update < refresh)
    {
      auto monitoring_request =  network::create_packet(network::MONITORING_REQUEST, station->serialize());
      auto response = network::packet(inet_addr(host.ipAddress), monitoring_request, timeout, port);
      if (response.status == network::SUCCESS)
      {
        station->GetStationTable()->update_retry(hostname, 0);
        station->GetStationTable()->update(hostname, AWAKEN, host.type);
      } 
      else if (host.status != ASLEEP) 
      {
        station->GetStationTable()->update_retry(hostname, host_info.retry_counter + 1);
        if (host_info.retry_counter + 1 >= max_retry)
          station->GetStationTable()->update(hostname, ASLEEP, host.type);
      }
    }
  }
  int sleep = get_option(options, OPT_SLEEP, 5);
	std::this_thread::sleep_for(std::chrono::seconds(sleep));
}

void *monitoring::process_request(Station *station, packet_t data, std::function<void(packet_t)> resolve) 
{
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