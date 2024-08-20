#include "include/replication.h"

#include <string>

#include "include/network.h"

using namespace replication;

void replication::replicate(service_params_t *params, const std::string& payload)
{
	auto station = params->station;
	auto station_table = params->station_table;
	if (station->GetType() == MANAGER)
	{
		station_table->mutex.lock();
		auto replication_request = network::create_packet(MessageType::REPLICATION_REQUEST, station->serialize(), station_table->clock, 0, payload);
		station_table->serialize(replication_request.table);
		replication_request.table_size = station_table->table.size();

		std::vector<in_addr_t> addrs;
		for (auto &host_pair : station_table->table)
		{
			auto host = host_pair.second.first;
			if (host.macAddress == station->GetMacAddress())
				continue;

			if (host.status != StationStatus::AWAKEN)
				continue;

			addrs.push_back(inet_addr(host.ipAddress));
		}

		auto future_responses = network::multicast(addrs, replication_request, params->logger, params->options);
		auto responses = future_responses.get();
		for (auto &response : responses)
		{
			if (response.second.status == network::SUCCESS)
				addrs.erase(std::remove(addrs.begin(), addrs.end(), response.first), addrs.end());
		}

		if (addrs.size() > 0)
		{
			future_responses = network::multicast(addrs, replication_request, params->logger, params->options);
			responses = future_responses.get();
			for (auto &response : responses)
			{
				if (response.second.status != network::SUCCESS)
				{
					char ipAddress[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &response.first, ipAddress, INET_ADDRSTRLEN);
					params->logger->error("Failed to replicate to ", ipAddress);
				}
			}
		}
		
		station_table->mutex.unlock();
	}
}

void *replication::process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close)
{
	auto station = params->station;
	auto station_table = params->station_table;

	if (station->GetType() == HOST)
	{
		if (station->GetManager() != NULL && station->GetManager()->GetMacAddress().compare(data.station.macAddress) == 0)
		{
			if (data.type == MessageType::REPLICATION_REQUEST && data.clock > station_table->clock)
			{
				auto response = network::create_packet(MessageType::REPLICATION_RESPONSE, station->serialize(), station_table->clock, 0);
				response.status = network::SUCCESS;
				resolve(response);

				StationTable::deserialize(station_table, data.table, data.table_size, data.clock);
    		std::this_thread::sleep_for(std::chrono::seconds(1));
				params->ui_lock.unlock();
				return 0;
			}
		}
	}
	close();
	return 0;
}