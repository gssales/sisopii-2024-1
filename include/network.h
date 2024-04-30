#ifndef _NETWORK_H
#define _NETWORK_H

#include <arpa/inet.h>

#include "include/station.h"

namespace network
{  
  const int PORT = 50505;

  struct packet
  {
    unsigned short seqn; //Número de sequência
    unsigned short length; //Comprimento do payload
    unsigned long timestamp; // Timestamp do dado
    unsigned short status; //Status da mensagem
    char message[255]; //Dados da mensagem
    station_serial station;
  };
  
  struct sockaddr_in socket_address(in_addr_t addr);

  void *tcp_server();
  void tcp_call_resolve(int sockfd, sockaddr_in client_addr, struct packet request_data, std::function<void(struct packet, std::function<void(struct packet)>)> callback);

  void *udp_server(Station *station);
  void udp_call_resolve(int sockfd, sockaddr_in client_addr, struct packet request_data, std::function<void(struct packet, std::function<void(struct packet)>)> callback);

  int open_socket(int sock_type);
	struct packet datagram(in_addr_t address, struct packet data);
	struct packet packet(in_addr_t address, struct packet data);

};

#endif