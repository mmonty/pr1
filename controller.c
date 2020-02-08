#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "utilities.h"

static void init_config(char *fn, struct sockaddr_in **swch_p, int *switch_num, int ***band_matr, int ***path_matr);
static void free_config(struct sockaddr_in *switches, int switch_num, int **band_matr, int **path_matr);
static void compute_path(int **path_matr, int **band_matr, int num);

int main(int argc, char *argv[])
{
	int 			s;
	char 			buf[512];
	struct sockaddr_in	peer;
	socklen_t 		peer_len = sizeof(struct sockaddr_in);
	int 			i,j;
	int			id;
	size_t			len;
	int 			count;

	struct sockaddr_in*	switches; 	//array of addrress of switches
	int 			switch_num;	//num of switches
	int**			band_matr;	//bandwidth matrix of network
	int**			path_matr;	//hop matrix of network
	struct timeval 		time1;
	struct timeval		trem;
        fd_set                  readfds;
	int*			mcount;


	if (argc != 4) {
		printf("usage: ./controller hostname portnumber config_file");
		exit(EXIT_FAILURE);
	}

	init_config(argv[3], &switches, &switch_num, &band_matr, &path_matr);

	s = sock_bind(AF_INET, SOCK_DGRAM, argv[1], argv[2]);
	mcount = calloc(switch_num, sizeof(*mcount));

	//gather initial requests;
	i = 0;
	while (i < switch_num) {
		if (recvfrom(s, buf, 2, 0, (struct sockaddr *) &peer, &peer_len) != 2) {
			fprintf(stderr, "Controller recvfrom failed or bad packet\n");
			continue;
		}
		if (buf[0] != REGISTER_REQUEST) {
			fprintf(stderr, "Controller recvfrom bad packet\n");
			continue;
		}
		id = buf[1];
		switches[id-1] = peer;
		i++;
	}

	for (i = 0; i < switch_num; i++) {
		print_sockaddr((struct sockaddr *)&switches[i]);
	}

	//send initial responses
	buf[0] = REGISTER_RESPONSE;
	buf[1] = switch_num;
	memcpy(&buf[switch_num+2], switches, sizeof(*switches)*switch_num);
	len = 2 + switch_num*sizeof(*switches) + switch_num;
	for (i = 0; i < switch_num; i++) {
		for (j = 0; j < switch_num; j++) {
			if (band_matr[i][j])
				buf[2+j] = 1;
			else
				buf[2+j] = 0;
		}
		sendto(s, buf, len, 0, (struct sockaddr *)&switches[i], peer_len);
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

			time_rem(&trem, &time1);
		}
		else {
		//increment and check
                        trem.tv_sec = KTIME;
                        trem.tv_usec = 0;
			
		}
	}	

	free(mcount);
	free_config(switches, switch_num, band_matr, path_matr);
	close(s);
	exit(EXIT_SUCCESS);
}

static void
traverse(int node, int source, int hop, int bneck, int been[], int **path_matr, int **band_matr, int num) {
	int i;
	int temp_bneck;

	if (bneck > been[node]){
		path_matr[source][node] = hop;
		been[node] = bneck;
	}
	
	for (i = 0; i < num; i++) 
		if (band_matr[node][i] > been[i]) 
			traverse(i, source, hop, band_matr[node][i], been, path_matr, band_matr, num);
}

static void 
compute_path(int **path_matr, int **band_matr, int num) {
	int i;
	int j;
	int k;
	int been[num];
	
	for (i = 0; i < num; i++) {
		been[i] = 0;
		for (j = 0; j < num; j++)
			path_matr[i][j] = 0;
	}


	for (i = 0; i < num; i++) {
		for (j = 0; j < num; j++) {
			if (band_matr[i][j] > 0)
				traverse(j, i, j, band_matr[i][j], been, path_matr, band_matr, num);
		}
		for (k = 0; k < num; k++)
			been[k] = 0;
	}
}

static void 
init_config(char *fn, struct sockaddr_in **swch_p, int *switch_num, int ***band_matr, int ***path_matr) {
	FILE *fp;
	int s1, s2, bw, delay;
	int i;

	fp = fopen(fn, "r");
	if (fp == NULL) {
		perror("controller could not open config file");
		exit(EXIT_FAILURE);
	}

	if (fscanf(fp, "%i ", &s1) != 1) {
		fprintf(stderr, "config file not formattted correctly\n");
		exit(EXIT_FAILURE);
	}
	*switch_num = s1;

	*swch_p = calloc(s1, sizeof(struct sockaddr_in));
	*band_matr = calloc(s1, sizeof(int *));
	for (i = 0; i < s1; i++) 
		(*band_matr)[i] = calloc(s1, sizeof(int));
	*path_matr = calloc(s1, sizeof(int *));
	for (i = 0; i < s1; i++) 
		(*path_matr)[i] = calloc(s1, sizeof(int));

	int rv;
	while ((rv=fscanf(fp, "%i %i %i %i ", &s1, &s2, &bw, &delay)) == 4) {
		printf("%d %d %d %d\n", s1, s2, bw, delay);
		(*band_matr)[s1-1][s2-1] = bw;
		(*band_matr)[s2-1][s1-1] = bw;
	}
	if (ferror(fp)) {
		fprintf(stderr, "error reading file\n");
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	return;
}

static void 
free_config(struct sockaddr_in *switches, int switch_num, int **band_matr, int **path_matr) {
	int i;

	for (i = 0; i < switch_num; i++) {
		free(band_matr[i]);
		free(path_matr[i]);
	}
	free(band_matr);
	free(path_matr);
	free(switches);
}
