#include "include/network.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

using namespace network;

struct packet network::udp_send(in_addr_t address, struct packet data)
{
  int sockfd = network::open_udp_socket();

  struct sockaddr_in sock_addr = socket_address(address);
  int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
  if (n < 0) 
    std::cout << "ERROR sending message" << std::endl;
    
  struct packet response;
  struct sockaddr_in from_addr;
  socklen_t from_addr_len = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd, &response, sizeof(struct packet), 0, (struct sockaddr *) &from_addr, &from_addr_len);
	if (n < 0)
  {
		printf("ERROR recvfrom");
    response.status = 400;
  }

	printf("Got an ack: %d\n", response.status);
	
	close(sockfd);
  return response;
}

int process_request(struct packet *request)
{
  request->status = 200;
  return 200;
}

int network::udp_server(/*queue, mutex*/)
{
  int sockfd = network::open_udp_socket();
  
  struct sockaddr_in bound_addr = socket_address(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) < 0) 
    std::cerr << "ERROR binding socket" << std::endl;

  while (1 /* station status != EXITING */)
  {
    struct sockaddr_in client_addr;
    struct packet client_data;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    Station client_station;

    int n = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (n > 0)
    {
      std::cout << "(UDP Server) Message Received: " << std::endl;
      std::cout << client_data.message << std::endl << std::endl;
      Station::deserialize(&client_station, client_data.station);
      client_station.print();

      /** Proccess Request */
      process_request(&client_data);

      n = sendto(sockfd, &client_data, sizeof(struct packet), 0,(struct sockaddr *) &client_addr, sizeof(struct sockaddr));
      if (n  < 0) 
        printf("ERROR on sendto");
    }
  }

  close(sockfd);
  return 0;
}

int network::open_udp_socket()
{
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    std::cerr << "ERROR opening socket" << std::endl;
      
  struct timeval timeout; // Needs a timeout to finish the program
  timeout.tv_sec = 10; // 10s timeout
  timeout.tv_usec = 500000; // 500ms timeout
  int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if (ret < 0)
    std::cout << "ERROR option timeout errno: " << strerror(errno) << std::endl;
      
  int broadcastEnable = 1;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  if (ret < 0)
    std::cout << "ERROR option broadcast" << std::endl;

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
