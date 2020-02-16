#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "utilities.h"

static void init_config(char *fn, struct sockaddr_in **swch_p, int *switch_num, int ***band_matr, int ***path_matr);
static void free_config(struct sockaddr_in *switches, int switch_num, int **band_matr, int **path_matr);
static void compute_path(int **path_matr, int **band_matr, int num);
static void print_path(int **path_matr, int num);
static void write_route(char * dhpair, int sw_ind, int **path_matr, int num);

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
	int 			recompute;

	struct sockaddr_in*	switches; 	//array of addrress of switches
	int 			switch_num;	//num of switches
	int**			band_matr;	//bandwidth matrix of network
	int**			path_matr;	//hop matrix of network
	struct timeval 		time1;
	struct timeval		trem;
	struct timeval		tcopy;
        fd_set                  readfds;
	int*			mcount;


	if (argc != 4) {
		printf("usage: ./controller hostname portnumber config_file\n");
		exit(EXIT_FAILURE);
	}

	init_config(argv[3], &switches, &switch_num, &band_matr, &path_matr);

	compute_path(path_matr, band_matr, switch_num);
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
		printf("Got a REGISTER_REQUEST from id:%d\n", id);
		i++;
	}

	//send initial responses
	buf[0] = REGISTER_RESPONSE;
	buf[1] = switch_num;
	memcpy(&buf[2 + switch_num + 2*switch_num], switches, sizeof(*switches)*switch_num);
	len = 2 + switch_num*sizeof(*switches) + 3*switch_num;
	for (i = 0; i < switch_num; i++) {
		for (j = 0; j < switch_num; j++) {
			if (band_matr[i][j])
				buf[2+j] = 1;
			else
				buf[2+j] = 0;
		}
		write_route(&buf[2+switch_num], i, path_matr, switch_num);
		
		sendto(s, buf, len, 0, (struct sockaddr *)&switches[i], peer_len);
		printf("sent initial REGISTER_RESPONSE to: %d\n", i+1);
	}

	print_path(path_matr, switch_num);


	//main loop
        tcopy.tv_sec = KTIME;
        tcopy.tv_usec = 0;
        while(1) {
        	FD_ZERO(&readfds);
        	FD_SET(s, &readfds);
		trem = tcopy;
                gettimeofday(&time1, NULL);
                count = select(s+1, &readfds, NULL, NULL, &trem);
		if (count) {
		//socket ready for reading
			recvfrom(s, buf, 512, 0, (struct sockaddr *) &peer, &peer_len);
			
			if (buf[0] == REGISTER_REQUEST) {
				id = buf[1];
				printf("Got a new REGISTER_REQUEST from id:%d\n", id);

				
				mcount[id-1] = 0;
				for (i = 0; i < switch_num; i++) {
					if (band_matr[i][id-1] < 0)
						band_matr[i][id-1] *= -1;
				}
				switches[id-1] = peer;

				compute_path(path_matr, band_matr, switch_num);

				buf[0] = REGISTER_RESPONSE;
				buf[1] = switch_num;
				memcpy(&buf[2 + switch_num + 2*switch_num], switches, sizeof(*switches)*switch_num);
				len = 2 + switch_num*sizeof(*switches) + 3*switch_num;
				for (j = 0; j < switch_num; j++) {
					if (band_matr[id-1][j])
						buf[2+j] = 1;
					else
						buf[2+j] = 0;
				}
				write_route(&buf[2+switch_num], id-1, path_matr, switch_num);
				sendto(s, buf, len, 0, (struct sockaddr *)&switches[id-1], peer_len);
				printf("sent new REGISTER_RESPONSE to: %d\n", id);

				len = 1 + 2*switch_num;
				buf[0] = ROUTE_UPDATE;
				for (i = 0; i < switch_num; i++) {
					write_route(&buf[1], i, path_matr, switch_num);
					sendto(s, buf, len, 0, (struct sockaddr *)&switches[i], peer_len);
				}
				print_path(path_matr, switch_num);
			}

			if (buf[0] == TOPOLOGY_UPDATE) {
				id = buf[1];
				printf("Got a TOPOLOGY_UPDATE from id:%d\n", id);

				recompute = 0;
				if (mcount[id-1] > MTIME) {
					for (i = 0; i < switch_num; i++) {
						if (band_matr[i][id-1] < 0) {
							band_matr[i][id-1] *= -1;
							recompute = 1;
						}
					}
				}
				mcount[id-1] = 0;
				for (i = 0; i < switch_num; i++) {
					if (band_matr[id-1][i] > 0 && buf[2+i] == 0) {
						band_matr[id-1][i] *= -1;
						recompute = 1;
					}
					if (band_matr[id-1][i] < 0 && buf[2+i] == 1 && mcount[i] <= MTIME) {
						band_matr[id-1][i] *= -1;
						recompute = 1;
					}
				}
				if (recompute) {
					compute_path(path_matr, band_matr, switch_num);
					len = 1 + 2*switch_num;
					buf[0] = ROUTE_UPDATE;
					for (i = 0; i < switch_num; i++) {
						write_route(&buf[1], i, path_matr, switch_num);
						sendto(s, buf, len, 0, (struct sockaddr *)&switches[i], peer_len);
					}
					print_path(path_matr, switch_num);
				}
			}
			time_rem(&tcopy, &time1);
		}
		else {
		//increment and check
                        tcopy.tv_sec = KTIME;
                        tcopy.tv_usec = 0;

			recompute = 0;
			for (i = 0; i < switch_num; i++) {
				if (mcount[i] > MTIME) {
					for (j = 0; j < switch_num; j++) {
						if (band_matr[j][i] > 0) {
							recompute = 1;
							band_matr[j][i] *= -1;
						}
					}
				}
				else
					mcount[i]++;
			}
			if (recompute) {
				compute_path(path_matr, band_matr, switch_num);
				len = 1 + 2*switch_num;
				buf[0] = ROUTE_UPDATE;
				for (i = 0; i < switch_num; i++) {
					write_route(&buf[1], i, path_matr, switch_num);
					sendto(s, buf, len, 0, (struct sockaddr *)&switches[i], peer_len);
				}
				print_path(path_matr, switch_num);
			}
		
			printf("\n");
		}
	}	

	free(mcount);
	free_config(switches, switch_num, band_matr, path_matr);
	close(s);
	exit(EXIT_SUCCESS);
}

static void 
write_route(char * dhpair, int sw_ind, int **path_matr, int num) {
	int i,j;

	for (j = 0; j < num; j++) {
		dhpair[2*j] = j;
		dhpair[2*j+1] = path_matr[sw_ind][j];
	}
	return;
}


static void 
compute_path(int **path_matr, int **band_matr, int num) {
	int i;
	int j;
	int k;
	int minb;
	int bw[num][num];
	
	for (i = 0; i < num; i++) {
		for (j = 0; j < num; j++) {
			
			bw[i][j] = band_matr[i][j] >= 0 ? band_matr[i][j] : 0;
			if (i == j)
				bw[i][i] = INT_MAX;
			path_matr[i][j] = j;
		}
	}

	for (k = 0; k < num; k++) {
		for (i = 0; i < num; i++) {	
			for (j = 0; j < num; j++) {	
				minb = bw[i][k] < bw[k][j] ? bw[i][k] : bw[k][j];
				if (bw[i][j] < minb) {
					bw[i][j] = minb;
					path_matr[i][j] = path_matr[i][k];
				}
			}
		}
	}
	for (i = 0; i < num; i++) {
		for (j = 0; j < num; j++) {
			if (bw[i][j] == 0)
				path_matr[i][j] = -1;
		}
	}
	return;
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

static void print_path(int **path_matr, int num) {
	int i, j;
	
	printf("new path: \n");
	for (i = 0; i < num; i++)
		for (j = 0; j < num; j++)
			printf("%d, %d : %d\n", i+1, j+1, path_matr[i][j]+1);
}

