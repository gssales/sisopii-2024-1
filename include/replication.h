#ifndef _REPLICATION_H
#define _REPLICATION_H

#include "include/network.h"

using namespace network;

namespace replication
{

	void *service(service_params_t *params);

	void proc_host(service_params_t *params);

	void proc_manager(service_params_t *params);

	void *process_request(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve);
}

#endif