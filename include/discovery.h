#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "include/station.h"
#include <functional>

#include "include/network.h"
#include "include/options_parser.h"

using namespace network;

namespace discovery
{

	void *service(option_t *options, Station *station);

	void proc_host(option_t *options, Station *station);

	void *process_request(Station *station, packet_t data, std::function<void(packet_t)> resolve);
}

#endif