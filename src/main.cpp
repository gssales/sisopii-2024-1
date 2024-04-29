#include <iostream>
#include <cstring>

#include "include/station.h"
#include "include/network.h"
#include "include/utils.h"

int main(int argc, const char *argv[]) {

	auto station = new Station();

	station->init();
	if (argc > 1 && strcmp(argv[1], "manager") == 0)
		station->SetType(MANAGER);

	station->print();

	if (station->GetType() == MANAGER) 
	{
		network::udp_server();
	}
	else
	{
		struct network::packet data;
		char message[255] = "Hey I'm the Participant";
		strcpy(data.message, message);
		data.seqn = 1;
		data.length = strlen(message);
		data.timestamp = now();
		data.status = 100;
		struct network::packet res = network::udp_send(INADDR_BROADCAST, data);
		std::cout << res.status << std::endl;
	}

	return 0;
}
