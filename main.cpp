#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define PORT 50000

int main(int argc, const char *argv[]) {
	
	std::cout << "Hello World" << std::endl;

	std::string type = "participant";
	if (argc > 1) {
		std::cout << "I am a " << argv[1] << std::endl;
		type = argv[1];

		system("hostname");
	}

	if (type.compare("manager") == 0) {

		int sockfd, n;
		socklen_t clilen;
		struct sockaddr_in serv_addr, cli_addr;
		char buf[256];
			
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
			std::cout << "ERROR opening socket" << std::endl;

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORT);
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		bzero(&(serv_addr.sin_zero), 8);    
		
		if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
			std::cout << "ERROR on binding" << std::endl;
		
		clilen = sizeof(struct sockaddr_in);
		
		while (1) {
			std::cout << "Receiving" << std::endl;
			/* receive from socket */
			n = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &cli_addr, &clilen);
			if (n < 0) 
				std::cout << "ERROR on recvfrom" << std::endl;
			std::cout << "Received a datagram: "<< buf << std::endl;
			
			/* send to socket */
			n = sendto(sockfd, "Got your message\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
			if (n  < 0) 
				std::cout << "ERROR on sendto" << std::endl;
		}

		close(sockfd);
	} 
	else {
    int sockfd, n;
		unsigned int length;
		struct sockaddr_in serv_addr, from;
		struct hostent *server;
		
		char buffer[256];
		if (argc < 2) {
			std::cout << "usage " << argv[0] << " hostname" << std::endl;
			exit(0);

		}
		
		server = gethostbyname("manager");
		if (server == NULL) {
			std::cout << "ERROR, no such host" << std::endl;
			exit(0);
		}	
		
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
			std::cout << "ERROR opening socket" << std::endl;
		
		serv_addr.sin_family = AF_INET;     
		serv_addr.sin_port = htons(PORT);    
		serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
		bzero(&(serv_addr.sin_zero), 8);  

		//std::cout << "Enter the message: " << std::endl;
		bzero(buffer, 256);
		//fgets(buffer, 256, stdin);
		char hostname[HOST_NAME_MAX];
		gethostname(hostname, HOST_NAME_MAX);
    sprintf(buffer, "This is a message from ");
		strcat(buffer, hostname);

		n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		if (n < 0) 
			std::cout << "ERROR sendto" << std::endl;
		
		length = sizeof(struct sockaddr_in);
		n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
		if (n < 0)
			std::cout << "ERROR recvfrom" << std::endl;

		std::cout << "Got an ack: " << buffer << std::endl;
		
		close(sockfd);
	}

	return 0;
}
