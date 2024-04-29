#ifndef _NETWORK_H
#define _NETWORK_H

#include <arpa/inet.h>

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
  };

  void tcp_server(/*queue, mutex*/);
  /*message*/ void tcp_send(/*message*/); // async

  int open_tcp_socket();

  int udp_server(/*queue, mutex*/);
  struct packet udp_send(in_addr_t address, struct packet data); // async

  int open_udp_socket();

  struct sockaddr_in socket_address(in_addr_t addr);

};

#endif