#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "utilities.h"

static void print_route(char *route, int id, int num);

int 
main(int argc, char *argv[])
{
	int s;
	char buf[512];
	int i;
	int count;
	int verbose = 0;
	char ignore[128] = {0};

	int id;
	struct addrinfo *	controller_addr;
        struct sockaddr_in      peer;
        socklen_t               peer_len = sizeof(struct sockaddr_in);
	struct sockaddr_in*	switches;
	char*			nbor;
	char*			live_nbors;
	int*			mcount;
	char*			route;
	int			switch_num;
	struct timeval		time1;
	struct timeval		trem;
	struct timeval		tcopy;
	fd_set			readfds;

	if (argc < 4) {
		printf("usage: ./switch controller_hostname controller_portnumber id [-v] [-f args]\n");
		exit(EXIT_FAILURE);
	}
	
	if (argc > 4) {
		i = 5;
		if (argv[4][1] == 'v') {
			verbose = 1;
			i = 6;
		}
		while (i < argc) {
			ignore[atoi(argv[i])-1] = 1;
			i++;
		}
	}
	
	id = atoi(argv[3]);
	s = sock_bind(AF_INET, SOCK_DGRAM, "0.0.0.0", NULL);
	controller_addr = get_hostaddr(argv[1], argv[2]);
	
	//initial request
	buf[0] = REGISTER_REQUEST;
	buf[1] = id;
	sendto(s, buf, 2, 0, controller_addr->ai_addr, controller_addr->ai_addrlen);
	printf("Sent REGISTER_REQUEST\n\n");
	

	//initial reply
	recvfrom(s, buf, 512, 0, NULL, NULL);
	if (buf[0] != REGISTER_RESPONSE) {
		fprintf(stderr, "bad initial response from controller\n");
		exit(EXIT_FAILURE);
	}
	printf("got REGISTER_RESPONSE\n\n");

	switch_num = buf[1];
	switches = calloc(switch_num, sizeof(*switches));
	nbor = calloc(switch_num, sizeof(*nbor));
	live_nbors = calloc(switch_num, sizeof(*live_nbors));
	mcount = calloc(switch_num, sizeof(*mcount));
	route = calloc(2*switch_num, sizeof(*route));
	memcpy(switches, &buf[2+switch_num+2*switch_num], switch_num*sizeof(*switches));
	memcpy(nbor, &buf[2], switch_num);
	memcpy(route, &buf[2+switch_num], 2*switch_num);
	memcpy(live_nbors, nbor, switch_num);

	//main loop
	trem.tv_sec = KTIME;
	trem.tv_usec = 0;
	while(1) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);
		tcopy = trem;
		gettimeofday(&time1, NULL);
		count = select(s+1, &readfds, NULL, NULL, &tcopy);
		if (count) {
		//socket ready for reading
			recvfrom(s, buf, 512, 0, (struct sockaddr *)&peer, &peer_len);
			
			if (buf[0] == KEEP_ALIVE) {
				i = buf[1] - 1;
				if (ignore[i])
					continue;
				if (verbose)
					printf("Got a KEEP_ALIVE from id:%d\n", i+1);
				if (live_nbors[i] == 0) {
					switches[i] = peer;
					printf("switch %d now reachable\n", i+1);
				}
				mcount[i] = 0;
				live_nbors[i] = 1;
			}	
			else if (buf[0] == ROUTE_UPDATE) {
				printf("Got a ROUTE_UPDATE from controller\n");
				memcpy(route, &buf[1], 2*switch_num);
				print_route(route, id, switch_num);
			}
			
			time_rem(&trem, &time1);
		}
		else {
		//timeout, time to transmit
			trem.tv_sec = KTIME;
			trem.tv_usec = 0;

			for (i = 0; i < switch_num; i++) {
				if (nbor[i]) {
					if (mcount[i] > MTIME) {
						if (live_nbors[i])
							printf("switch %d now unreachable\n", i+1);
						live_nbors[i] = 0;
					}
					else
						mcount[i]++;
				}
			}
	
			for (i = 0; i < switch_num; i++) {
				if (nbor[i] && live_nbors[i] && !ignore[i]) {
					buf[0] = KEEP_ALIVE;
					buf[1] = id;
					sendto(s, buf, 2, 0, (struct sockaddr *) &switches[i], peer_len);
				}
			}
			buf[0] = TOPOLOGY_UPDATE;
			buf[1] = id;
			memcpy(&buf[2], live_nbors, switch_num);
			sendto(s, buf, 2+switch_num, 0, controller_addr->ai_addr, peer_len);

			if (verbose)
				printf("\n");
		}
	}

	free(mcount);
	free(switches);
	free(nbor);
	free(live_nbors);
	free(route);
	freeaddrinfo(controller_addr);
	close(s);
	exit(EXIT_SUCCESS);
}

static void 
print_route(char *route, int id, int num) {
	int i;

	printf("new route:\n");
	for (i = 0; i < num; i++) 
		printf("%d, %d : %d\n", id, i+1, route[2*i+1]+1);
}
