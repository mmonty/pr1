#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define REGISTER_REQUEST 1 	// 1 : 1, msgid : switch id
#define REGISTER_RESPONSE 2	// 1 : 1 : switch_num : : 2*switch_num : sizeof(struct sockaddr_in)*switch_num ,  msgid : number of switches : neighbors 1,0 : destination,hop pairs : switch address data 
#define ROUTE_UPDATE 3		// 1 : 2 * switch_num , msgid : desination,hop pairs
#define KEEP_ALIVE 4		// 1 : 1, msgid : switch_id
#define TOPOLOGY_UPDATE 5	// 1 : 1 : switch_num , msgid : switch_id : list of live neighbors(1,0)

#ifndef KTIME
#define KTIME 1
#endif

#ifndef MTIME
#define MTIME 6
#endif

int sock_bind(int domain, int type, const char *hostname, const char *servname);
void print_sockaddr(struct sockaddr* addr);
struct addrinfo *get_hostaddr(const char* hostname, const char *servname);
void time_rem(struct timeval *result, struct timeval *last);
