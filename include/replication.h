#ifndef _REPLICATION_H
#define _REPLICATION_H

#include "include/network.h"

using namespace network;

namespace replication
{
	void *process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close);

	void replicate(service_params_t *params, const std::string& payload = "");
}

#endif