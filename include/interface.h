#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "include/station.h"
#include "include/options_parser.h"

namespace interface
{
	void *interface(option_t *options, Station *station);
}

#endif