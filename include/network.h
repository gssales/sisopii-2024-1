#ifndef _NETWORK_H
#define _NETWORK_H

#include <arpa/inet.h>

#include "include/station.h"

namespace network
{  
  const int PORT = 50505;

	enum MessageType: unsigned short
	{
		UNKNOWN,
		DISCOVERY_REQUEST,
		DISCOVERY_RESPONSE,
		LEAVING
	};

	enum RequestStatus: unsigned short
	{
		PENDING,
		SUCCESS,
		FAIL
	};

  typedef struct packet
  {
		MessageType type;
    unsigned short seqn; //Número de sequência
    unsigned long timestamp; // Timestamp do dado
		unsigned long clock;
    RequestStatus status; //Status da mensagem
    unsigned short length; //Comprimento do payload
    char message[255]; //Dados da mensagem
    station_serial station;
  } packet_t;
  
  struct sockaddr_in socket_address(in_addr_t addr);

  void *tcp_server();
  void tcp_call_resolve(int sockfd, sockaddr_in client_addr, Station *station, packet_t request_data, std::function<void(Station*, packet_t, std::function<void(packet_t)>)> callback);

  void *udp_server(Station *station);
  void udp_call_resolve(int sockfd, sockaddr_in client_addr, Station *station, packet_t request_data, std::function<void(Station*, packet_t, std::function<void(packet_t)>)> callback);

  int open_socket(int sock_type);
	packet_t datagram(in_addr_t address, packet_t data);
	packet_t packet(in_addr_t address, packet_t data);

	packet_t create_packet(MessageType type, station_serial station, short clock = 0, short seqn = 0, const std::string& payload = "");
};

#endif