#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "include/station.h"
#include "include/service.h"

#define CMD_EXIT "EXIT"
#define CMD_REFRESH "refresh"
#define CMD_WAKEUP "WAKEUP"

namespace interface
{
	void *interface(service_params_t *params);

	void *command(service_params_t *params);
}

#endif