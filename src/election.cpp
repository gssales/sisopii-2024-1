#include "include/election.h"

#include "include/network.h"

packet_t request_manager_status(service_params_t *params)
{
  auto station = params->station;
  auto manager = station->GetManager();
  auto check_request = network::create_packet(network::ONLINE, station->serialize());
  auto response = network::datagram(inet_addr(manager->GetIpAddress().c_str()), check_request, params->logger, params->options, true);
  return response;
}

void *election::service(service_params_t *params)
{
  auto station = params->station;
  int sleep = get_option(params->options, OPT_SLEEP, 5);
  while (station->GetStatus() != EXITING)
  {
    if (station->GetType() == HOST && station->GetManager() != NULL)
    {
      auto response = request_manager_status(params);
      if (response.type != network::ONLINE || response.status != network::SUCCESS)
        start_election(params);
    }

    std::this_thread::sleep_for(std::chrono::seconds(sleep));
  }
  return 0;
}

void *election::process_request_udp(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve)
{
  auto station = params->station;

  if (data.type == network::ONLINE)
  {
    auto response = network::create_packet(network::ONLINE, station->serialize(), station->GetClock());
    response.status = network::SUCCESS;
    resolve(response);
  }
	return 0;
}

void *election::process_request_tcp(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close)
{
  // auto station = params->station;

	close();
	return 0;
}

void election::start_election(service_params_t *params)
{
  auto station = params->station;
  auto station_table = params->station_table;
  auto logger = params->logger;
  auto options = params->options;

  if (station->GetType() == CANDIDATE)
    return;

  // Stations calls this when it suspects the manager is down
  if (station->GetManager() != NULL)
  {
    auto response = request_manager_status(params);
    if (response.type == network::ONLINE && response.status == network::SUCCESS)
      return;
  }

  // Manager is down, start election
  station->SetType(CANDIDATE);
  station->SetStatus(ELECTING);
  station->SetManager(NULL);

  if (station_table->table.size() == 0)
  {
    auto check_request = network::create_packet(network::ONLINE, station->serialize());
    auto response = network::datagram(INADDR_BROADCAST, check_request, logger, options, true);
    if (response.status != network::SUCCESS)
    {
      station->SetType(MANAGER);
      station->SetStatus(AWAKEN);
      station->SetManager(NULL);
		  station_table->insert(station->GetHostname(), station->serialize());
      return;
    }
    else 
    {
      // ???
    }
    
  }
  else 
  {
    // ???
  }
}