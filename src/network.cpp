#include "include/network.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <functional>
#include <thread>
#include <chrono>

using namespace network;

void *process_request(struct packet request, std::function<void(struct packet)> resolve)
{
  request.status = 200;
	
	resolve(request);
	return 0;
}


void *tcp_server(/*subservice*/)
{
	// TODO implement non blocking accept
	
  // int sockfd = open_socket(SOCK_DGRAM);

  // struct sockaddr_in bound_addr = socket_address(INADDR_ANY);
  // if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) == -1) 
  //   std::cerr << "ERROR binding socket: " << strerror(errno) << std::endl;
    
  // if (listen(sockfd, 10) == -1) 
  //   std::cerr << "ERROR listen: " << strerror(errno) << std::endl;

  // while (1 /* station status != EXITING */)
  // {
  //   struct sockaddr_in client_addr;
  //   socklen_t client_addr_len = sizeof(struct sockaddr_in);
  //   if (accept())

  //   struct packet client_data;

  //   int n = recvfrom(this->sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
  //   if (n > 0)
  //   {
  //     std::cout << "(UDP Server) Message Received: " << std::endl;
  //     std::cout << client_data.message << std::endl << std::endl;
      
  //     Station client_station;
  //     Station::deserialize(&client_station, client_data.station);
  //     client_station.print();

	// 		this->call_resolve(client_addr, client_data, process_request);
  //   }
  // }
	return 0;
}

struct packet network::packet(in_addr_t address, struct packet data)
{
  int sockfd = open_socket(SOCK_STREAM);

  struct sockaddr_in sock_addr = socket_address(address);
  if (connect(sockfd, (const struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0)
		std::cerr << "ERROR on connect: " << strerror(errno) << std::endl;

  int n = write(sockfd, &data, sizeof(data));
  if (n < 0)
		std::cerr << "ERROR on write: " << strerror(errno) << std::endl;

  struct packet response;
	n = read(sockfd, &response, sizeof(struct packet));
	if (n < 0)
  {
		std::cerr << "ERROR on read: " << strerror(errno) << std::endl;
    response.status = 400;
  }

  close(sockfd);
  return response;
}


void *network::udp_server(Station *station)
{
  int sockfd = open_socket(SOCK_DGRAM);

  struct sockaddr_in bound_addr = socket_address(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) == -1) 
    std::cerr << "ERROR binding socket: " << strerror(errno) << std::endl;

  while (1 /* station status != EXITING */)
  {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    struct packet client_data;

    int n = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (n > 0)
    {
			if (client_addr.sin_addr.s_addr == station->GetInAddr())
				continue;

      std::cout << "(UDP Server) Message Received: " << std::endl;
      std::cout << client_data.message << std::endl << std::endl;
      
      Station client_station;
      Station::deserialize(&client_station, client_data.station);
      client_station.print();

			udp_call_resolve(sockfd, client_addr, client_data, process_request);
    }
  }
}

void network::udp_call_resolve(int sockfd, sockaddr_in client_addr, struct packet request_data, std::function<void(struct packet, std::function<void(struct packet)>)> callback)
{
	auto resolve_callback = [sockfd, client_addr](struct packet response_data) 
	{
		int n = sendto(sockfd, &response_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, sizeof(struct sockaddr));
		if (n  < 0) 
			std::cerr << "ERROR on sendto: " << strerror(errno) << std::endl;
	};

	std::thread process_thread (callback, request_data, resolve_callback);
	process_thread.detach();
}

struct packet network::datagram(in_addr_t address, struct packet data)
{
  int sockfd = open_socket(SOCK_DGRAM);

  struct sockaddr_in sock_addr = socket_address(address);
  int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
  if (n < 0) 
		std::cerr << "ERROR on sendto: " << strerror(errno) << std::endl;
    
  struct packet response;
  struct sockaddr_in from_addr;
  socklen_t from_addr_len = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd, &response, sizeof(struct packet), 0, (struct sockaddr *) &from_addr, &from_addr_len);
	if (n < 0)
  {
		std::cerr << "ERROR on recvfrom: " << strerror(errno) << std::endl;
    response.status = 400;
  }

  close(sockfd);
  return response;
}

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

struct sockaddr_in network::socket_address(in_addr_t addr)
{
  struct sockaddr_in address;
  address.sin_family = AF_INET;     
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = addr;
  memset(&(address.sin_zero), 0, 8);
  return address;
}
