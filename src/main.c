#include <sys/socket.h>	//struct sockaddr
#include <sys/time.h>	//gettimeofday()
#include <netdb.h>		// getaddrinfo(), freeaddrinfo()
#include <stdio.h>		// printf()
#include <string.h>		// memset(), strcmp()
#include <arpa/inet.h> 	//inet_ntop(),htons()
#include <stdlib.h>		//exit()
#include <netinet/ip_icmp.h> // struct icmphdr
#include <netinet/ip.h> // struct iphdr
#include <unistd.h> 	// getpid()
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
	char send_packet[PACKET_SIZE];
	struct icmphdr *icmp;
	char *host;
	int received;
	int lost;
	double rtt_min;
	double rtt_max;
	double rtt_sum;
	double rtt_sum_sq;
	unsigned short pid;
}	PingData;

PingData data;
bool stop = 0;

int ft_flag(int ac, char **av, char **host) {
	int check_v; // 0 = normal, 2 = verbose

	check_v = 0; // valeur initiale
	for(int i = 1 ; i < ac ; i++)
	{
		if (av[i] && !*host && av[i][0] != '-')
			*host = av[i]; // premier argument non-option = host
		if (av[i] && av[i][0] == '-')
		{
			if (!*host && av[i][1] == '\0')
				return(printf("ping: unknown host\n"), 1); // '-' seul -> erreur
			if (av[i][1] == '?'){
				printf("Usage: ft_ping [-v] destination\n-v    verbose output\n-?    display this help\n"); // aide
				exit(0);
			}
				else if (av[i][1] == 'v')
				check_v = 2; // mode verbose
			else if (av[i][1] != '\0')
				return(printf("ping: invalid option -- '%c'\nTry 'ping --help' or 'ping --usage' for more information.\n", av[i][1]), 64); // option invalide
		}
	}
	if (*host == NULL)
		return(printf("ping: missing host operand\nTry 'ping --help' or 'ping --usage' for more information.\n"), 64); // pas de host fourni
	return (check_v);
}

void ft_icmp_builder(PingData *data)
{
	memset(data->send_packet, 0, PACKET_SIZE);
	
	data->icmp = (struct icmphdr *)data->send_packet;
	data->icmp->type = ICMP_ECHO; // 8
	data->icmp->code = 0;
	data->pid = getpid() & 0xFFFF;
	data->icmp->un.echo.id = htons(data->pid); //convertie le pid du pc vers la langue du reseau (little -> big endian)
}

void ft_icmp_checksum(PingData *data)
{
	// Calcul du checksum uniquement sur la taille de l'en-tête ICMP (8 octets)
	data->icmp->checksum = 0;
	uint32_t sum = 0;
	uint16_t *ptr = (uint16_t *)data->send_packet;
	size_t icmp_len = PACKET_SIZE; // 8 octets
	for (size_t i = 0; i < icmp_len / 2; i++)
		sum += ptr[i];
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (sum & 0xFFFF) + (sum >> 16);
	data->icmp->checksum = ~((uint16_t)sum);
}

void handle_sigint()
{
	stop = 1;
}

void ft_stat(double deltaT)
{
	if (data.rtt_min > deltaT)
		data.rtt_min = deltaT;
	if (data.rtt_max < deltaT)
		data.rtt_max = deltaT;
	data.rtt_sum += deltaT;
	data.rtt_sum_sq += deltaT*deltaT;
}

int main(int ac, char **av) {
	int check_v;

	data.received = 0;
	data.rtt_max = 0;
	data.rtt_min = DBL_MAX;
	data.rtt_sum = 0;
	data.rtt_sum_sq = 0;
	data.host = NULL;
	check_v = ft_flag(ac, av, &data.host);
	if (check_v != 0 && check_v != 2)
		return (check_v);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(data.host, NULL, &hints, &data.res) != 0)
		return(printf("ping: unknown host\n"), 1);

	struct sockaddr_in *dest_addr = (struct sockaddr_in *)data.res->ai_addr;
	char addr_ip[INET_ADDRSTRLEN];
	if (!inet_ntop(AF_INET, &dest_addr->sin_addr, addr_ip, INET_ADDRSTRLEN))
		return (perror("inet_ntop"), freeaddrinfo(data.res), 1);

	
	data.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (data.sockfd == -1)
		return(perror("socket"), 1);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	setsockopt(data.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	ft_icmp_builder(&data);
	uint16_t sequence = 0;
	int transmitted = 0;
	char recv_packet[PACKET_SIZE]; // Buffer séparé pour la réception
	if (check_v == 0)
        printf("PING %s (%s): %ld data bytes\n", data.host, addr_ip, PACKET_SIZE - sizeof(struct icmphdr));
	else if (check_v == 2)
		printf("PING %s (%s): %ld data bytes, id 0x%04x = %d \n", data.host, addr_ip, PACKET_SIZE - sizeof(struct icmphdr), data.pid, data.pid);
	
	signal(SIGINT, handle_sigint);
	while (!stop)
	{

		data.icmp->un.echo.sequence = htons(sequence);

		gettimeofday((struct timeval *)(data.send_packet + sizeof(struct icmphdr)), NULL);
		struct timeval tv1 = *(struct timeval *)(data.send_packet + sizeof(struct icmphdr));
		ft_icmp_checksum(&data);
	
		ssize_t send = sendto(data.sockfd, data.send_packet, PACKET_SIZE, 0, (struct sockaddr *)data.res->ai_addr, sizeof(struct sockaddr_in));
		if (send == -1)
		{
			perror("sendto");
			sleep(1);
			continue;
		}
		transmitted++;
		
		struct sockaddr_in src_addr;
		socklen_t addrlen = sizeof(src_addr);
		ssize_t recv_bytes = recvfrom(data.sockfd, recv_packet, PACKET_SIZE, 0, (struct sockaddr *)&src_addr, &addrlen);
		if (recv_bytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				printf("Request timeout for icmp_seq %d\n", sequence);
			else
				perror("recv:");
		}
		if (!inet_ntop(AF_INET, &src_addr.sin_addr, addr_ip, INET_ADDRSTRLEN))
			return (perror("inet_ntop"), freeaddrinfo(data.res), 1);
        if (recv_bytes > 0)
		{
			data.received++;
			// recv_packet: [ IP_HEADER | ICMP_HEADER | ICMP_DATA ] ← paquet reçu complet
            struct iphdr *iphdr = (struct iphdr *)recv_packet; //contient les information ip du packet recu  / ihl = Internet Header Length 
            struct icmphdr *icmphdr = (struct icmphdr *)(recv_packet + iphdr->ihl * 4);
			struct timeval tv2;
			gettimeofday(&tv2, NULL);
			double deltaT = (((tv2.tv_sec - tv1.tv_sec) * 1000.0) + ((tv2.tv_usec - tv1.tv_usec) / 1000.0));
			ft_stat(deltaT);
			//convertie la sequence du reseau vers la langue du pc (big endian -> little)
			printf("%ld bytes from %s: icmp_seq=%d, ttl=%d, time=%.3f ms\n", 
				recv_bytes, addr_ip, ntohs(icmphdr->un.echo.sequence), iphdr->ttl, deltaT);
		}
		sequence++;
		if (transmitted > 0)
			data.lost = ((transmitted - data.received) * 100) / transmitted;
		sleep(1);
	}
	double avg = 0;
	double variance = 0;
	double stddev = 0;
	if (data.received == 0)
	{
		data.rtt_min = 0;
		data.rtt_max = 0;
	}
	else
	{
		avg = data.rtt_sum / data.received;
		variance = (data.rtt_sum_sq / data.received) - (avg * avg);
		if (variance < 0)
			variance = 0;
		stddev = sqrt(variance);
	}
	printf("--- %s ping statistics ---\n", data.host);
	printf("%d packets transmitted, %d packets received, %d%% packet loss\n", transmitted, data.received, data.lost);	
	printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", data.rtt_min, avg, data.rtt_max, stddev);

	freeaddrinfo(data.res);
    return (0);
}
