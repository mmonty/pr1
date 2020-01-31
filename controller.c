#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
	int s;
	int addr_err;
	struct addrinfo hints;
	struct addrinfo *res;

	if (argc != 3) {
		printf("usage: ./controller hostname portnumber")
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;    
	hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = 0;    
	hints.ai_protocol = 0;        
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	addr_err = getaddrinfo(argv[1], argv[2], &hints, &res);
	if (addr_err != 0) {
		fprintf(stderr, "controller failed getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror("Controller failed to allocate socket");
		exit(EXIT_FAILURE);
	}

	if (bind(s, res->ai_addr, res->ai_addrlen) == -1) {
		perror("Controller failed to bind socket");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);



	close(s);
	exit(EXIT_SUCCESS);
}
