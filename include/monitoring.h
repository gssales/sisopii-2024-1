#ifndef _MONITORING_H
#define _MONITORING_H

#include "include/station.h"
#include <functional>

#include "include/network.h"

using namespace network;

namespace monitoring
{

	void *service(service_params_t *params);

	void proc_manager(service_params_t *params);

	void *process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close);
}

#endif