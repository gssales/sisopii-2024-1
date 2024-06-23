#include "include/monitoring.h"

#include <iostream>
#include <thread>
#include "include/utils.h"

using namespace monitoring;

void *monitoring::service(Station *station)
{
  while (station->GetStatus() != EXITING)
  {
    /**
     * Na estação manager, o serviço de monitoramento é ativo
     * itera sobre os hosts, atualizando sleep status
     */
    if (station->GetType() == MANAGER)
      proc_manager(station);
      
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
void monitoring::proc_manager(Station *station)
{
  for (auto &host_pair : station->GetStationTable()->getValues())
  {
    auto host = host_pair.first;
    auto host_info = host_pair.second;
    if ((host.status != ASLEEP && host_info.last_update < now() -10000) 
        || (host.status == ASLEEP && host_info.last_update < now() - 30000))
    {
      auto monitoring_request =  network::create_packet(network::MONITORING_REQUEST, station->serialize());
      auto response = network::packet(inet_addr(host.ipAddress), monitoring_request);
      if (response.status == network::SUCCESS)
      {
        station->GetStationTable()->update_retry(host.hostname, 0);
        station->GetStationTable()->update(host.hostname, AWAKEN, host.type);
      } 
      else if (host.status != ASLEEP) 
      {
        station->GetStationTable()->update_retry(host.hostname, host_info.retry_counter + 1);
        if (host_info.retry_counter > 3)
          station->GetStationTable()->update(host.hostname, ASLEEP, host.type);
      }
    }
  }
	std::this_thread::sleep_for(std::chrono::seconds(5));
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