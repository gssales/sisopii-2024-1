#include <iostream>
#include <cstring>
#include <thread>

#include "include/station.h"
#include "include/network.h"
#include "include/utils.h"

int main(int argc, const char *argv[]) {

	auto station = new Station();

	station->init();
	if (argc > 1 && strcmp(argv[1], "manager") == 0)
		station->SetType(MANAGER);
	station->print();


	auto udp_thread = std::thread(&network::udp_server, station);
	
	if (station->GetType() != MANAGER) 
	{
		struct network::packet data;
		char message[255] = "Hey I'm the Participant";
		strcpy(data.message, message);
		data.seqn = 1;
		data.length = strlen(message);
		data.timestamp = now();
		data.status = 100;
		data.station = station->serialize();
		auto res = network::datagram(INADDR_BROADCAST, data);
		std::cout << "Response Status: " << res.status << std::endl;
	}

	udp_thread.join();

	return 0;
}
