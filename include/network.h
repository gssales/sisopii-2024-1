#ifndef _NETWORK_H
#define _NETWORK_H

#include <arpa/inet.h>
#include <future>

#include "include/station.h"
#include "include/options_parser.h"
#include "include/service.h"

#define UDP_PORT 50505 // default, can be changed using -p-dgram <port> option
#define TCP_PORT 50506 // default, can be changed using -p-stream <port> option

namespace network
{  

	enum MessageType: unsigned short
	{
		UNKNOWN,
		DISCOVERY_REQUEST,
		DISCOVERY_RESPONSE,
		MONITORING_REQUEST,
		MONITORING_RESPONSE,
		LEAVING,
		REPLICATION_REQUEST,
		REPLICATION_RESPONSE
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
		station_serial table[5];
		unsigned short table_size;
  } packet_t;
  
  struct sockaddr_in socket_address(in_addr_t addr, const int port);

  void *tcp_server(service_params_t *params);
  void tcp_call_resolve(int client_sockfd, sockaddr_in client_addr, service_params_t *params, packet_t request_data, std::function<void(service_params_t*, packet_t, std::function<void(packet_t)>, std::function<void()>)> callback);

  void *udp_server(service_params_t *params);
  void udp_call_resolve(int sockfd, sockaddr_in client_addr, service_params_t *params, packet_t request_data, std::function<void(service_params_t*, packet_t, std::function<void(packet_t)>)> callback);

  int open_socket(int sock_type, int timeout_sec, Logger *logger);
	packet_t datagram(in_addr_t address, packet_t data, Logger *logger, options_t *options, bool read_response = true);
	packet_t packet(in_addr_t address, packet_t data, Logger *logger, options_t *options, bool read_response = true);

	packet_t create_packet(MessageType type, station_serial station, short clock = 0, short seqn = 0, const std::string& payload = "");
	std::string print_packet(packet_t packet);

	std::future<std::vector<std::pair<in_addr_t,packet_t>>> multicast(std::vector<in_addr_t> addrs, packet_t data, Logger *logger, options_t *options, bool read_response = true);
	std::future<std::vector<std::pair<station_serial,packet_t>>> multicast(std::vector<station_serial> stations, packet_t data, Logger *logger, options_t *options, bool read_response = true);
};

#endif