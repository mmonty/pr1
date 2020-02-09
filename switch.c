#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "utilities.h"

int 
main(int argc, char *argv[])
{
	int s;
	char buf[512];
	int i;
	int count;

	int id;
	struct addrinfo *	controller_addr;
	struct sockaddr_in*	switches;
	char*			nbor;
	char*			live_nbors;
	int*			mcount;
	char*			route;
	int			switch_num;
	struct timeval		time1;
	struct timeval		trem;
	fd_set			readfds;

	if (argc != 4) {
		printf("usage: ./switch controller_hostname controller_portnumber id");
		exit(EXIT_FAILURE);
	}
	
	id = atoi(argv[3]);
	s = sock_bind(AF_INET, SOCK_DGRAM, "0.0.0.0", NULL);
	controller_addr = get_hostaddr(argv[1], argv[2]);
	
	//initial request
	buf[0] = REGISTER_REQUEST;
	buf[1] = id;
	sendto(s, buf, 2, 0, controller_addr->ai_addr, controller_addr->ai_addrlen);

	//initial reply
	recvfrom(s, buf, 512, 0, NULL, NULL);
	if (buf[0] != REGISTER_RESPONSE) {
		fprintf(stderr, "bad initial response from controller\n");
		exit(EXIT_FAILURE);
	}
	switch_num = buf[1];
	switches = calloc(switch_num, sizeof(*switches));
	nbor = calloc(switch_num, sizeof(*nbor));
	live_nbors = calloc(switch_num, sizeof(*live_nbors));
	mcount = calloc(switch_num, sizeof(*mcount));
	route = calloc(2*switch_num, sizeof(*route));
	memcpy(switches, &buf[2+switch_num], switch_num*sizeof(*switches));
	memcpy(nbor, &buf[2], switch_num);
	memcpy(live_nbors, nbor, switch_num);

	for (i = 0; i < switch_num; i++) {
		print_sockaddr((struct sockaddr *) &switches[i]);
	}

	//main loop
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	trem.tv_sec = KTIME;
	trem.tv_usec = 0;
	while(1) {
		gettimeofday(&time1, NULL);
		count = select(1, &readfds, NULL, NULL, &trem);
		if (count) {
		//socket ready for reading
			recvfrom(s, buf, 512, 0, NULL, NULL);
			
			if (buf[0] == KEEP_ALIVE) {
				i = buf[1] - 1;
				mcount[i] = 0;
				live_nbors[i] = 1;
			}	
			else if (buf[0] == ROUTE_UPDATE) {
				memcpy(route, &buf[1], 2*switch_num);
			}
			
			time_rem(&trem, &time1);
		}
		else {
		//timeout, time to transmit
			trem.tv_sec = KTIME;
			trem.tv_usec = 0;

			for (i = 0; i < switch_num; i++) {
				if (nbor[i]) {
					if (mcount[i] > MTIME)
						live_nbors[i] = 0;
					else
						mcount[i]++;
				}
			}
	
			for (i = 0; i < switch_num; i++) {
				if (nbor[i]) {
					buf[0] = KEEP_ALIVE;
					buf[1] = id;
					sendto(s, buf, 2, 0, (struct sockaddr *) &switches[i], controller_addr->ai_addrlen);
				}
			}
			buf[0] = TOPOLOGY_UPDATE;
			buf[1] = id;
			memcpy(&buf[2], live_nbors, switch_num);
			sendto(s, buf, 2+switch_num, 0, controller_addr->ai_addr, controller_addr->ai_addrlen);

		}
	}

	free(mcount);
	free(switches);
	free(nbor);
	free(live_nbors);
	freeaddrinfo(controller_addr);
	close(s);
	exit(EXIT_SUCCESS);
}

