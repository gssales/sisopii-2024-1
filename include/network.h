#ifndef _NETWORK_H
#define _NETWORK_H

#include <arpa/inet.h>

#include "include/station.h"

namespace network
{  
  const int PORT = 50505;
	int tcp_socket_fd = -1;
	int udp_socket_fd = -1;

  struct packet
  {
    unsigned short seqn; //Número de sequência
    unsigned short length; //Comprimento do payload
    unsigned long timestamp; // Timestamp do dado
    unsigned short status; //Status da mensagem
    char message[255]; //Dados da mensagem
    station_serial station;
  };

  int open_tcp_socket();
  void tcp_server(/*queue, mutex*/);
  /*message*/ void tcp_send(/*message*/); // async

	class NetworkService
	{
	public:
		void *server(/*subservice*/);
		packet request(in_addr_t server_address, packet request_data);

		void call_resolve(int sockfd, struct sockaddr_in client_addr, struct packet *request_data);
	}

	class UDPService
	{
	public:
		void *server(/*subservice*/);
		packet request(in_addr_t server_address, packet request_data);

		void call_resolve(int sockfd, struct sockaddr_in client_addr, struct packet *request_data);
	}


  int open_udp_socket();
  void *udp_server(/*queue, mutex*/);
  struct packet udp_send(in_addr_t address, struct packet data); // async


  struct sockaddr_in socket_address(in_addr_t addr, int port);

	void spawn_resolve_detached(int sockfd, struct sockaddr_in client_addr, struct packet *request_data);

};

#endif