#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define REGISTER_REQUEST 1 	// 1 : 1, msgid : switch id
#define REGISTER_RESPONSE 2	// 1 : 1 : switch_num : sizeof(struct sockaddr_in)*switch_num,  msgid : number of switches : neighbors 1,0 : switch address data
#define ROUTE_UPDATE 3
#define KEEP_ALIVE 4
#define TOPOLOGY_UPDATE 5

int sock_bind(int domain, int type, const char *hostname, const char *servname);
void print_sockaddr(struct sockaddr* addr);
struct addrinfo *get_hostaddr(const char* hostname, const char *servname);

struct swch {
	int id;
	struct sockaddr_in addr;
};

