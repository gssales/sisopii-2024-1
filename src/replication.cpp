#include "include/replication.h"

#include <string>

#include "include/network.h"

using namespace replication;

void replication::replicate(service_params_t *params, const std::string& payload)
{
	auto station = params->station;
	auto station_table = params->station_table;
	if (station->GetType() == MANAGER && station_table->has_update)
	{
		auto replication_request = network::create_packet(MessageType::REPLICATION_REQUEST, station->serialize(), station_table->clock, 0, payload);
		station_table->serialize(replication_request.table);
		replication_request.table_size = station_table->table.size();
  	
		for (auto &host_pair : station_table->clone())
		{
			auto hostname = host_pair.first;
			auto host = host_pair.second.first;

			if (host.macAddress == station->GetMacAddress())
				continue;

			if (host.status != StationStatus::AWAKEN)
				continue;

			auto response = network::packet(inet_addr(host.ipAddress), replication_request, params->logger, params->options);
			if (response.status != network::SUCCESS)
			{
				// Retry
				response = network::packet(inet_addr(host.ipAddress), replication_request, params->logger, params->options);
				if (response.status != network::SUCCESS)
				{
					params->logger->error("Failed to replicate to ", host.ipAddress);
				}
			}
		}
	}
}

void *replication::process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close)
{
	auto station = params->station;
	auto station_table = params->station_table;

	if (station->GetType() == HOST)
	{
		if (station->GetManager() == NULL || station->GetManager()->GetMacAddress().compare(data.station.macAddress) == 0)
		{
			if (data.type == MessageType::REPLICATION_REQUEST && data.clock > station_table->clock)
			{
				StationTable::deserialize(station_table, data.table, data.table_size);
				station_table->clock = data.clock;
				station_table->has_update = true;
				params->ui_lock.unlock();

				auto response = network::create_packet(MessageType::REPLICATION_RESPONSE, station->serialize(), station_table->clock, 0);
				response.status = network::SUCCESS;
				resolve(response);
				return 0;
			}
		}
	}
	close();
	return 0;
}