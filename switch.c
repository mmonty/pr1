#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static int sock_bind(int domain, int type, const char *hostname, const char *servname);

int main(int argc, char *argv[])
{
	int s;

	if (argc != 3) {
		printf("usage: ./controller hostname portnumber");
		exit(EXIT_FAILURE);
	}

	s = sock_bind(AF_INET, SOCK_DGRAM, argv[1], argv[2]);

	

	close(s);
	exit(EXIT_SUCCESS);
}

static int 
sock_bind(int domain, int type, const char *hostname, const char *servname) 
{
	int s;
	int addr_err;
	struct addrinfo hints;
	struct addrinfo *res;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;    
	hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = 0;    
	hints.ai_protocol = 0;        
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	addr_err = getaddrinfo(hostname, servname, &hints, &res);
	if (addr_err != 0) {
		fprintf(stderr, "controller failed getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	s = socket(domain, type, 0);
	if (s == -1) {
		perror("Controller failed to allocate socket");
		exit(EXIT_FAILURE);
	}

	if (bind(s, res->ai_addr, res->ai_addrlen) == -1) {
		perror("Controller failed to bind socket");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	return s;
}
