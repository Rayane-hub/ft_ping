#ifndef PING_H
#define PING_H

#include <sys/socket.h>	//struct sockaddr
#include <sys/time.h>	//gettimeofday()
#include <netdb.h>		// getaddrinfo(), freeaddrinfo()
#include <stdio.h>		// printf()
#include <string.h>		// memset(), strcmp()
#include <arpa/inet.h> 	//inet_ntop(),htons()
#include <stdlib.h>		//exit()
#include <netinet/ip_icmp.h> // struct icmphdr
#include <netinet/ip.h> // struct iphdr
#include <unistd.h> 	// getpid()signum
#include <errno.h> 		// errno
#include <signal.h>		//signal()
#include <float.h>		//DBL_MAX
#include <stdbool.h>	//bool
#include <math.h> 		//sqrt()

#define INET_ADDRSTRLEN 16
#define PACKET_SIZE 64

typedef struct ping_data {
	struct addrinfo *res;
	int sockfd;
	char    send_packet[PACKET_SIZE];
    char    addr_ip[INET_ADDRSTRLEN];
	struct icmphdr  *icmp;
	char    *host;
    int transmitted;
	int received;
	int lost;
	double  rtt_min;
	double  rtt_max;
	double  rtt_sum;
	double  rtt_sum_sq;
	unsigned short  pid;
}	PingData;

void    icmp_builder(PingData *data);
void    icmp_checksum(PingData *data);
void    compute_stat(PingData *data, double deltaT);
void    print_stat(PingData *data);
int 	check_flag(int ac, char **av, char **host);
int 	resolve_host(PingData *data);
int 	init_socket(PingData *data);
void    init_data(PingData *data);
void    ping_send_loop(PingData *data);
void    ping_recv(struct timeval tv1, uint16_t sequence, PingData *data);
void    handle_sigint();

#endif