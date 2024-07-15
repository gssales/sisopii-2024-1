#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "include/station.h"
#include "include/service.h"

#define CMD_EXIT "exit"
#define CMD_REFRESH "refresh"

namespace interface
{
	void *interface(service_params_t *params);

	void *command(service_params_t *params);
}

#endif