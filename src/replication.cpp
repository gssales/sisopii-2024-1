#include "include/replication.h"

using namespace replication;

void *replication::service(service_params_t *params)
{
	auto station = params->station;
	while (station->GetStatus() != EXITING)
	{
		/*
		 * Na estação manager, o serviço de discory é passivo
		 * Termina thread
		 */
		if (station->GetType() == MANAGER)
			break;
		
		/*
		 * Na estação participante, o serviço de discory é ativo
		 * Busca manager
		 */
		if (station->GetType() == HOST)
			proc_host(params);
	}
	return 0;
}
