#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	int			switch_num;
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
	memcpy(switches, &buf[2+switch_num], switch_num*sizeof(*switches));
	memcpy(nbor, &buf[2], switch_num);

	for (i = 0; i < switch_num; i++) {
		print_sockaddr((struct sockaddr *) &switches[i]);
	}

	//main loop
	free(switches);
	free(nbor);
	freeaddrinfo(controller_addr);
	close(s);
	exit(EXIT_SUCCESS);
}

