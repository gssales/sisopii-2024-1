#ifndef _MONITORING_H
#define _MONITORING_H

#include "include/station.h"
#include <functional>

#include "include/network.h"

using namespace network;

namespace monitoring
{

	void *service(Station *station);

	void proc_manager(Station *station);

	void *process_request(Station *station, packet_t data, std::function<void(packet_t)> resolve);
}

#endif