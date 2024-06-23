#include "include/network.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/time.h>
#include <errno.h>
#include <functional>
#include <thread>
#include <chrono>

#include "include/discovery.h"
#include "include/monitoring.h"
#include "include/utils.h"

packet_t create_error_packet(char* message)
{
  packet_t p;
  p.type = UNKNOWN;
  p.timestamp = now();
  p.status = FAIL;
  p.length = strlen(message);
  strcpy(p.message, message);
  return p;
}

using namespace network;

void *network::tcp_server(Station *station)
{
  int sockfd = open_socket(SOCK_STREAM);

  struct sockaddr_in bound_addr = socket_address(INADDR_ANY, TCP_PORT);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) == -1) 
    std::cerr << "ERROR binding socket: " << strerror(errno) << std::endl;
    
  if (listen(sockfd, 10) == -1) 
    std::cerr << "ERROR listen: " << strerror(errno) << std::endl;

  while (1 /* station status != EXITING */)
  {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);

    int client_sockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_sockfd != -1)
    {
      if (client_addr.sin_addr.s_addr == station->GetInAddr())
        continue;

      struct packet client_data;
      int n = read(client_sockfd, &client_data, sizeof(struct packet));
      if (n > 0)
      {
        switch (client_data.type)
        {
        case MONITORING_REQUEST:
        case MONITORING_RESPONSE:
          tcp_call_resolve(client_sockfd, client_addr, station, client_data, monitoring::process_request);
          break;
        
        default:
          break;
        }
      }
    }
  }
  close(sockfd);
}

void network::tcp_call_resolve(int client_sockfd, sockaddr_in client_addr, Station *station, packet_t request_data, std::function<void(Station*, packet_t, std::function<void(packet_t)>)> callback)
{
	auto resolve_callback = [client_sockfd, client_addr](packet_t response_data) 
	{
    int n = write(client_sockfd, &response_data, sizeof(packet_t));
		if (n  < 0) 
			std::cerr << "ERROR on writing to socket: " << strerror(errno) << std::endl;
    
    close(client_sockfd);
	};

	std::thread process_thread (callback, station, request_data, resolve_callback);
	process_thread.detach();
}

packet_t network::packet(in_addr_t address, packet_t data)
{
  int sockfd = open_socket(SOCK_STREAM);

  struct sockaddr_in sock_addr = socket_address(address, TCP_PORT);
  if (connect(sockfd, (const struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0) 
  {
		std::cerr << "ERROR on connect: " << strerror(errno) << std::endl;
    close(sockfd);
    return create_error_packet(strerror(errno));
  }

  int n = write(sockfd, &data, sizeof(data));
  if (n < 0) 
  {
		std::cerr << "ERROR on write: " << strerror(errno) << std::endl;
    close(sockfd);
    return create_error_packet(strerror(errno));
  }

  packet_t response;
	n = read(sockfd, &response, sizeof(packet_t));
	if (n < 0)
  {
		std::cerr << "ERROR on read: " << strerror(errno) << std::endl;
    response = create_error_packet(strerror(errno));
  }

  close(sockfd);
  return response;
}


void *network::udp_server(Station *station)
{
  int sockfd = open_socket(SOCK_DGRAM);

  struct sockaddr_in bound_addr = socket_address(INADDR_ANY, UDP_PORT);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) == -1) 
    std::cerr << "ERROR binding socket: " << strerror(errno) << std::endl;

  while (1 /* station status != EXITING */)
  {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    packet_t client_data;

    int n = recvfrom(sockfd, &client_data, sizeof(packet_t), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (n > 0)
    {
			if (client_addr.sin_addr.s_addr == station->GetInAddr())
				continue;

			switch (client_data.type)
			{
			case DISCOVERY_REQUEST:
			case DISCOVERY_RESPONSE:
				udp_call_resolve(sockfd, client_addr, station, client_data, discovery::process_request);
				break;
			
			default:
				break;
			}
    }
  }
  close(sockfd);
}

void network::udp_call_resolve(int sockfd, sockaddr_in client_addr, Station *station, packet_t request_data, std::function<void(Station*, packet_t, std::function<void(packet_t)>)> callback)
{
	auto resolve_callback = [sockfd, client_addr](packet_t response_data) 
	{
		int n = sendto(sockfd, &response_data, sizeof(packet_t), 0, (struct sockaddr *) &client_addr, sizeof(struct sockaddr));
		if (n  < 0) 
			std::cerr << "ERROR on sendto: " << strerror(errno) << std::endl;
	};

	std::thread process_thread (callback, station, request_data, resolve_callback);
	process_thread.detach();
}

packet_t network::datagram(in_addr_t address, packet_t data)
{
  int sockfd = open_socket(SOCK_DGRAM);

  struct sockaddr_in sock_addr = socket_address(address, UDP_PORT);
  int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
	if (n < 0)
  {
  	std::cerr << "ERROR on sendto: " << strerror(errno) << std::endl;
    close(sockfd);
    return create_error_packet(strerror(errno));
  }
    
  packet_t response;
  struct sockaddr_in from_addr;
  socklen_t from_addr_len = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd, &response, sizeof(packet_t), 0, (struct sockaddr *) &from_addr, &from_addr_len);
	if (n < 0)
  {
		std::cerr << "ERROR on recvfrom: " << strerror(errno) << std::endl;
    response = create_error_packet(strerror(errno));
  }

  close(sockfd);
  return response;
}

packet_t network::create_packet(MessageType type, station_serial station, short clock /*= 0*/, short seqn /*= 0*/, const std::string& payload /*= ""*/)
{
	packet_t p;
	p.type = type;
	p.seqn = seqn;
	p.timestamp = now();
	p.clock = clock;
	p.status = PENDING;
	p.length = sizeof(payload);
	strcpy(p.message, payload.c_str());
	p.station = station;
	return p;
};

int network::open_socket(int sock_type)
{
  int sockfd;
  if ((sockfd = socket(AF_INET, sock_type, 0)) == -1) 
    std::cerr << "ERROR opening socket: " << strerror(errno) << std::endl;
      
  struct timeval timeout; // Needs a timeout to finish the program
  timeout.tv_sec = 10; // 10s timeout
  timeout.tv_usec = 500000; // 500ms timeout
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    std::cout << "ERROR option timeout: " << strerror(errno) << std::endl;
      
  int broadcastEnable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1)
    std::cout << "ERROR option broadcast: " << strerror(errno) << std::endl;

  return sockfd;
}

struct sockaddr_in network::socket_address(in_addr_t addr, const int port)
{
  struct sockaddr_in address;
  address.sin_family = AF_INET;     
  address.sin_port = htons(port);
  address.sin_addr.s_addr = addr;
  memset(&(address.sin_zero), 0, 8);
  return address;
}
