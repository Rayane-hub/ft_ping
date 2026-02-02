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


#define INET_ADDRSTRLEN 16
#define PACKET_SIZE 64

typedef struct ping_data {
	struct addrinfo *res;
	int sockfd;
	char send_packet[PACKET_SIZE];
	struct icmphdr *icmp;
}PingData;

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

void	ft_icmp_builder(PingData *data)
{
	memset(data->send_packet, 0, PACKET_SIZE);
	
	data->icmp = (struct icmphdr *)data->send_packet;
	data->icmp->type = ICMP_ECHO; // 8
	data->icmp->code = 0;
	unsigned short pid = getpid() & 0xFFFF;
	data->icmp->un.echo.id = htons(pid); //convertie le pid du pc vers la langue du reseau (little -> big endian)
}

void ft_icmp_checksum(PingData *data)
{
	// Calcul du checksum uniquement sur la taille de l'en-tête ICMP (8 octets)
	data->icmp->checksum = 0;
	uint32_t sum = 0;
	uint16_t *ptr = (uint16_t *)data->send_packet;
	size_t icmp_len = sizeof(struct icmphdr); // 8 octets
	for (size_t i = 0; i < icmp_len / 2; i++)
		sum += ptr[i];
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (sum & 0xFFFF) + (sum >> 16);
	data->icmp->checksum = ~((uint16_t)sum);
}

int main(int ac, char **av) {
	int check_v;
	char *host = NULL;
	PingData data;

	check_v = ft_flag(ac, av, &host);
	if (check_v != 0 && check_v != 2)
		return (check_v);

	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(host, NULL, &hints, &data.res) != 0)
		return(printf("ping: unknown host\n"), 1);

	struct sockaddr_in *dest_addr = (struct sockaddr_in *)data.res->ai_addr;
	char addr_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &dest_addr->sin_addr, addr_ip, INET_ADDRSTRLEN);
	printf("PING %s (%s): ? data bytes\n", host, addr_ip);
	
	data.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (data.sockfd == -1)
		return(printf("err socket\n"), 1);

	ft_icmp_builder(&data);
	uint16_t sequence = 0;
	char recv_packet[PACKET_SIZE]; // Buffer séparé pour la réception
	while (1)
	{
		data.icmp->un.echo.sequence = htons(sequence);

		gettimeofday((struct timeval *)(data.send_packet + sizeof(struct icmphdr)), NULL);
		//struct timeval *tv1 = (struct timeval *)(data.send_packet + sizeof(struct icmphdr));
		ft_icmp_checksum(&data);
		
		ssize_t send = sendto(data.sockfd, data.send_packet, PACKET_SIZE, 0, (struct sockaddr *)data.res->ai_addr, sizeof(struct sockaddr_in));
		if (send == -1)
			perror("sendto");
		
		struct sockaddr_in src_addr;
		socklen_t addrlen = sizeof(src_addr);
		ssize_t recv_bytes = recvfrom(data.sockfd, recv_packet, PACKET_SIZE, 0, (struct sockaddr *)&src_addr, &addrlen);
		if (recv_bytes == -1)
			perror("recv:");
		inet_ntop(AF_INET, &src_addr.sin_addr, addr_ip, INET_ADDRSTRLEN);
        if (recv_bytes > 0) 
		{
			// recv_packet: [ IP_HEADER | ICMP_HEADER | ICMP_DATA ] ← paquet reçu complet
            struct iphdr *iphdr = (struct iphdr *)recv_packet; //contient les information ip du packet recu  / ihl = Internet Header Length 
            struct icmphdr *icmphdr = (struct icmphdr *)(recv_packet + iphdr->ihl * 4);

			//gettimeofday((struct timeval *)(data.send_packet + sizeof(struct icmphdr)), NULL);
			//struct timeval *tv2 = (struct timeval*)(data.send_packet + sizeof(struct icmphdr));
			//time_t deltaT = tv2->tv_sec - tv1->tv_sec;
			
			//convertie la sequence du reseau vers la langue du pc (big endian -> little)
			printf("%ld bytes from %s: icmp_seq=%d, ttl=%d, time=? ms\n", recv_bytes, addr_ip, ntohs(icmphdr->un.echo.sequence), iphdr->ttl);
		}
		sequence++;
		sleep(1);
	}
	if (check_v == 0)
                printf("mode normal\n");
	else if (check_v == 2)
			printf("mode verbose\n");
	freeaddrinfo(data.res);
    return (0);
}
