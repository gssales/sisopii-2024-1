#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "include/station.h"
#include <functional>

#include "include/service.h"
#include "include/network.h"

using namespace network;

namespace discovery
{

	void *service(service_params_t *params);

	void proc_host(service_params_t *params);

	void *process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve);
}

#endif