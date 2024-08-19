#ifndef _ELECTION_H
#define _ELECTION_H

#include "include/network.h"

using namespace network;

namespace election
{
	void *service(service_params_t *params);

	void *process_request_udp(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve);
	void *process_request_tcp(service_params_t *params, packet_t data, std::function<void(packet_t)> resolve, std::function<void()> close);

	void start_election(service_params_t *params);
}

#endif