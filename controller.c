#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static int sock_bind(int domain, int type, const char *hostname, const char *servname);
static void print_sockaddr(struct sockaddr *addr);

int main(int argc, char *argv[])
{
	int 		s;
	char 		hello[6];
	struct sockaddr	peer;
	socklen_t 	peer_len = sizeof(struct sockaddr_in);

	if (argc != 3) {
		printf("usage: ./controller hostname portnumber");
		exit(EXIT_FAILURE);
	}

	s = sock_bind(AF_INET, SOCK_DGRAM, argv[1], argv[2]);

	//test
	recvfrom(s, hello, 6, 0, &peer, &peer_len);
		
	printf("received from:\n");
	print_sockaddr(&peer);
	printf("message: \n");
	fwrite(hello, 1, 6, stdout);

	close(s);
	exit(EXIT_SUCCESS);
}

static void
print_sockaddr(struct sockaddr *addr) {

        struct sockaddr_in *ipname;
        char ipstr[INET_ADDRSTRLEN];

        ipname = (void *)addr;
        inet_ntop(AF_INET, &ipname->sin_addr, ipstr, INET_ADDRSTRLEN);
        printf("address:%s\n", ipstr);

        return;
}

static int 
sock_bind(int domain, int type, const char *hostname, const char *servname) 
{
	int s;
	int addr_err;
	struct addrinfo hints;
	struct addrinfo *res;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = domain;    
	hints.ai_socktype = type; 
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
