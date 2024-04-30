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

	class TCPService
	{
  private:
    int sockfd = -1;

	public:
		void *server(/*subservice*/);
		struct packet request(in_addr_t server_address, struct packet request_data);

		void call_resolve(int sockfd, struct sockaddr_in client_addr, struct packet *request_data);
	};

	class UDPService
	{
  private:
    int sockfd = -1;

	public:
    UDPService();
    ~UDPService();

		void *server(/*subservice*/);

		void call_resolve(sockaddr_in client_addr, struct packet request_data, std::function<void(struct packet, std::function<void(struct packet)>)> callback);
	};
  void *udp_server();
  void udp_call_resolve(int sockfd, sockaddr_in client_addr, struct packet request_data, std::function<void(struct packet, std::function<void(struct packet)>)> callback);

  int open_socket(int sock_type);
	struct packet datagram(in_addr_t address, struct packet data);
	struct packet packet(in_addr_t address, struct packet data);

};

#endif