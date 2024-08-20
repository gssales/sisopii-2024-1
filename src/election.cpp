#include "include/election.h"

#include "include/network.h"
#include "include/replication.h"

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
      if (response.status != network::SUCCESS)
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
  auto station = params->station;

  if (data.type == network::ELECTION_REQUEST)
  {
    auto response = network::create_packet(network::ELECTION_RESPONSE, station->serialize());
    response.status = network::SUCCESS;
    resolve(response);

    start_election(params);
  }
  if (data.type == network::ELECTION_VICTORY)
  {
    auto response = network::create_packet(network::ELECTION_RESPONSE, station->serialize());
    response.status = network::SUCCESS;
    resolve(response);

    station->SetType(HOST);
    station->SetStatus(AWAKEN);

    if (station->GetManager() == NULL || station->GetManager()->GetMacAddress().compare(data.station.macAddress) != 0)
    {
      auto manager = new Station();
      Station::deserialize(manager, data.station);
      station->SetManager(manager);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    params->ui_lock.unlock();
  }

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

  // Manager is down, start election
  station->SetType(CANDIDATE);
  station->SetStatus(ELECTING);
  station->SetManager(NULL);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  params->ui_lock.unlock();

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
      std::this_thread::sleep_for(std::chrono::seconds(1));
      params->ui_lock.unlock();
      return;
    }
    else 
    {
      // ???
    }
    
  }
  else 
  {
    auto list = station_table->list(station->GetPid());
    std::vector<station_serial> stations;
    if (list.size() == 0)
    {
      station->SetType(MANAGER);
      station->SetStatus(AWAKEN);
      station->SetManager(NULL);
      station_table->insert(station->GetHostname(), station->serialize());
      std::this_thread::sleep_for(std::chrono::seconds(1));
      params->ui_lock.unlock();

      auto victory_response = network::create_packet(network::ELECTION_VICTORY, station->serialize());
      auto list = station_table->list(0);
      for (auto &host : list)
      {
        if (host.first.macAddress == station->GetMacAddress())
          continue;
        stations.push_back(host.first);
      }
      auto respose = network::multicast(stations, victory_response, logger, options);
      respose.get();
      return;
    }

    auto election_request = network::create_packet(network::ELECTION_REQUEST, station->serialize());

    for (auto &host : list)
      stations.push_back(host.first);
    auto responses = network::multicast(stations, election_request, logger, options, true);

    for (auto &response : responses.get())
    {
      if (response.second.type == network::ELECTION_RESPONSE && response.second.status == network::SUCCESS)
      {
        station->SetStatus(WAITING_MANAGER);
        params->ui_lock.unlock();
        break;
      }
    }

    if (station->GetStatus() == ELECTING)
    {
      station->SetType(MANAGER);
      station->SetStatus(AWAKEN);
      station->SetManager(NULL);
		  station_table->insert(station->GetHostname(), station->serialize());
      std::this_thread::sleep_for(std::chrono::seconds(1));
      params->ui_lock.unlock();
      
      auto victory_response = network::create_packet(network::ELECTION_VICTORY, station->serialize());
      auto list = station_table->list(0);
      for (auto &host : list)
      {
        if (host.first.macAddress == station->GetMacAddress())
          continue;
        stations.push_back(host.first);
      }
      auto respose = network::multicast(stations, victory_response, logger, options);
      respose.get();

      replication::replicate(params, "Manager elected");
      return;
    }
  }
}