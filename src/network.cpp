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
#include "include/replication.h"
#include "include/utils.h"
#include <unistd.h>

std::string MessageType_to_string(MessageType type)
{
  switch (type)
  {
  case DISCOVERY_REQUEST:
    return "DISCOVERY_REQUEST";
  case DISCOVERY_RESPONSE:
    return "DISCOVERY_RESPONSE";
  case MONITORING_REQUEST:
    return "MONITORING_REQUEST";
  case MONITORING_RESPONSE:
    return "MONITORING_RESPONSE";
  case LEAVING:
    return "LEAVING";
  case REPLICATION_REQUEST:
    return "REPLICATION_REQUEST";
  case REPLICATION_RESPONSE:
    return "REPLICATION_RESPONSE";
  default:
    return "UNKNOWN";
  }
}

std::string RequestStatus_to_string(RequestStatus status)
{
  switch (status)
  {
  case PENDING:
    return "PENDING";
  case SUCCESS:
    return "SUCCESS";
  case FAIL:
    return "FAIL";
  default:
    return "UNKNOWN";
  }
}

packet_t create_error_packet(char* message)
{
  packet_t p;
  p.type = UNKNOWN;
  p.timestamp = now();
  p.status = FAIL;
  p.length = strlen(message);
  strcpy(p.message, message);
  p.table_size = 0;
  return p;
}

packet_t create_empty_packet()
{
  packet_t p;
  p.type = UNKNOWN;
  p.timestamp = now();
  p.status = SUCCESS;
  p.length = 0;
  p.table_size = 0;
  return p;
}

using namespace network;

void *network::tcp_server(service_params_t *params)
{
  auto options = params->options;
  auto station = params->station;
  auto logger = params->logger;
  
  int timeout = get_option(options, OPT_TIMEOUT, 1); // default 1 second
  int sockfd = open_socket(SOCK_STREAM, timeout, logger);
	if (sockfd == -1)
		return 0;

  int port = get_option(options, OPT_PORT_STREAM, TCP_PORT);
  struct sockaddr_in bound_addr = socket_address(INADDR_ANY, port);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) == -1) {
    logger->error("binding socket: ", strerror(errno));
		close(sockfd);
		return 0;
	}
    
	if (listen(sockfd, 10) == -1){
		logger->error("listen: ", strerror(errno));
		close(sockfd);
		return 0;
	}

  while (station->GetStatus() != EXITING)
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
          tcp_call_resolve(client_sockfd, client_addr, params, client_data, monitoring::process_request);
          break;

				case REPLICATION_REQUEST:
				case REPLICATION_RESPONSE:
					tcp_call_resolve(client_sockfd, client_addr, params, client_data, replication::process_request);
					break;
        
        default:
          break;
        }
      }
    }
  }
  close(sockfd);
	return 0;
}

void network::tcp_call_resolve(int client_sockfd, sockaddr_in client_addr, service_params_t *params, packet_t request_data, std::function<void(service_params_t*, packet_t, std::function<void(packet_t)>, std::function<void()>)> callback)
{
	auto resolve_callback = [client_sockfd, client_addr, params](packet_t response_data) 
	{
    auto logger = params->logger;
    int n = write(client_sockfd, &response_data, sizeof(packet_t));
		if (n  < 0)
      logger->error("on writing to socket: ", strerror(errno));
    
    close(client_sockfd);
	};

	auto close_sock = [client_sockfd]() 
	{
    close(client_sockfd);
	};

	std::thread process_thread (callback, params, request_data, resolve_callback, close_sock);
	process_thread.detach();
}

packet_t network::packet(in_addr_t address, packet_t data, Logger *logger, options_t *options, bool read_response /*= true*/)
{
	int timeout = get_option(options, OPT_TIMEOUT, 1);
	int port = get_option(options, OPT_PORT_STREAM, TCP_PORT);

  int sockfd = open_socket(SOCK_STREAM, timeout, logger);
	if (sockfd == -1)
		return create_error_packet(strerror(errno));

  struct sockaddr_in sock_addr = socket_address(address, port);
  if (connect(sockfd, (const struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0) 
  {
		logger->error("on connect: ", strerror(errno));
    close(sockfd);
    return create_error_packet(strerror(errno));
  }

  int n = write(sockfd, &data, sizeof(data));
  if (n < 0) 
  {
		logger->error("on write: ", strerror(errno));
    close(sockfd);
    return create_error_packet(strerror(errno));
  }

  packet_t response =  create_empty_packet();
	if (read_response)
	{
		n = read(sockfd, &response, sizeof(packet_t));
		if (n < 0)
		{
			logger->error("on read: ", strerror(errno));
			response = create_error_packet(strerror(errno));
		}
	}

  close(sockfd);
  return response;
}


void *network::udp_server(service_params_t *params)
{
  auto options = params->options;
  auto station = params->station;
  auto logger = params->logger;

  int timeout = get_option(options, OPT_TIMEOUT, 1); // default 1 second
  int sockfd = open_socket(SOCK_DGRAM, timeout, logger);
	if (sockfd == -1)	
		return 0;

  int port = get_option(options, OPT_PORT_DGRAM, UDP_PORT);
  struct sockaddr_in bound_addr = socket_address(INADDR_ANY, port);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) == -1) {
    logger->error("binding socket: ", strerror(errno));
		close(sockfd);
		return 0;
	}

  while (station->GetStatus() != EXITING)
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
			case LEAVING:
				udp_call_resolve(sockfd, client_addr, params, client_data, discovery::process_request);
				break;
			
			default:
				break;
			}
    }
  }
  close(sockfd);
	return 0;
}

void network::udp_call_resolve(int sockfd, sockaddr_in client_addr, service_params_t *params, packet_t request_data, std::function<void(service_params_t*, packet_t, std::function<void(packet_t)>)> callback)
{
	auto resolve_callback = [sockfd, client_addr, params](packet_t response_data) 
	{
    auto logger = params->logger;
		int n = sendto(sockfd, &response_data, sizeof(packet_t), 0, (struct sockaddr *) &client_addr, sizeof(struct sockaddr));
		if (n  < 0)
      logger->error("on sendto: ", strerror(errno));
	};

	std::thread process_thread (callback, params, request_data, resolve_callback);
	process_thread.detach();
}

packet_t network::datagram(in_addr_t address, packet_t data, Logger *logger, options_t *options, bool read_response /*= true*/)
{
	int timeout = get_option(options, OPT_TIMEOUT, 1);
	int port = get_option(options, OPT_PORT_DGRAM, UDP_PORT);

  int sockfd = open_socket(SOCK_DGRAM, timeout, logger);
	if (sockfd == -1)
		return create_error_packet(strerror(errno));

  struct sockaddr_in sock_addr = socket_address(address, port);
  int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
	if (n < 0)
  {
  	logger->error("on sendto: ", strerror(errno));
    close(sockfd);
    return create_error_packet(strerror(errno));
  }
    
  packet_t response = create_empty_packet();
	if (read_response)
	{
		struct sockaddr_in from_addr;
		socklen_t from_addr_len = sizeof(struct sockaddr_in);
		n = recvfrom(sockfd, &response, sizeof(packet_t), 0, (struct sockaddr *) &from_addr, &from_addr_len);
		if (n < 0)
		{
			logger->error("on recvfrom: ", strerror(errno));
			response = create_error_packet(strerror(errno));
		}
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
  p.table_size = 0;
	return p;
};

std::string network::print_packet(packet_t packet)
{
  std::string str = "Type: " + MessageType_to_string(packet.type) + "; ";
  str += "Timestamp: " + std::to_string(packet.timestamp) + "; ";
  str += "Clock: " + std::to_string(packet.clock) + "; ";
  str += "Status: " + RequestStatus_to_string(packet.status) + "; ";
  str += "Message: " + std::string(packet.message) + "; ";
  str += "Table Size: " + std::to_string(packet.table_size) + "; ";
  return str;
}

std::string socket_type_to_string(int sock_type)
{
	switch (sock_type)
	{
	case SOCK_STREAM:
		return "SOCK_STREAM";
	case SOCK_DGRAM:
		return "SOCK_DGRAM";
	default:
		return "UNKNOWN";
	}
}

int network::open_socket(int sock_type, int timeout_sec, Logger *logger)
{
  int sockfd;
  if ((sockfd = socket(AF_INET, sock_type, 0)) == -1) {
    logger->error("opening socket: ", strerror(errno));
		logger->error(" socket type: " + socket_type_to_string(sock_type), strerror(errno));
		return -1;
	}

  struct timeval timeout; // Needs a timeout to finish the program
  timeout.tv_sec = timeout_sec;
  timeout.tv_usec = 1; // 1us timeout
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    logger->error("option rcv timeout: ", strerror(errno));
      
  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1)
    logger->error("option snd timeout: ", strerror(errno));
      

  int broadcastEnable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1) {
    logger->error("option broadcast: ", strerror(errno));
		close(sockfd);
		return -1;
	}

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
