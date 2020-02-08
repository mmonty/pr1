#include "utilities.h"

void
time_rem(struct timeval *result, struct timeval *last) {
	struct timeval current;

	gettimeofday(&current, NULL);

	if (current.tv_usec < last->tv_usec) {
		current.tv_sec = current.tv_sec - last->tv_sec - 1;
		current.tv_usec = 1000000 - last->tv_usec + current.tv_usec;
	}
	else {
		current.tv_sec = current.tv_sec - last->tv_sec;
		current.tv_usec = current.tv_usec - last->tv_usec;
	}

	if (result->tv_sec < current.tv_sec) {
		result->tv_sec = 0;
		result->tv_usec = 0;
		return;
 	}
	if (result->tv_usec < current.tv_usec) {
		if (result->tv_sec <= current.tv_sec) {
			result->tv_sec = 0;
			result->tv_usec = 0;
			return;
		}
		result->tv_sec = result->tv_sec - current.tv_sec - 1;
		result->tv_usec = 1000000 - current.tv_usec + result->tv_usec;
	}
	else {
		result->tv_sec = result->tv_sec - current.tv_sec;
		result->tv_usec = result->tv_usec - current.tv_usec;
	}
}

struct addrinfo * 
get_hostaddr(const char* hostname, const char *servname) {
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
		fprintf(stderr, "failed getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	return res;
}

void
print_sockaddr(struct sockaddr *addr) {

	struct sockaddr_in *ipname;
	char ipstr[INET_ADDRSTRLEN];

	ipname = (void *)addr;
	inet_ntop(AF_INET, &ipname->sin_addr, ipstr, INET_ADDRSTRLEN);
	printf("address:%s ", ipstr);
	printf("port:%d\n", ntohs(ipname->sin_port));
	return;
}

int 
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
		fprintf(stderr, "Switch failed getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	s = socket(domain, type, 0);
	if (s == -1) {
		perror("Switch failed to allocate socket");
		exit(EXIT_FAILURE);
	}

	if (bind(s, res->ai_addr, res->ai_addrlen) == -1) {
		perror("Switch failed to bind socket");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	return s;
}
