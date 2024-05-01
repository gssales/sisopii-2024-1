#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "include/station.h"
#include <functional>

#include "include/network.h"

using namespace network;

namespace discovery
{

	void *service(Station *station);

	void proc_host(Station *station);

	void *process_request(Station *station, packet_t data, std::function<void(packet_t)> resolve);
}

#endif