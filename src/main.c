#include "ping.h"

PingData data;
bool stop = 0;

int check_flag(int ac, char **av, char **host) {
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

void handle_sigint()
{
	stop = 1;
}

void init_data()
{
	data.received = 0;
	data.transmitted = 0;
	data.rtt_max = 0;
	data.rtt_min = DBL_MAX;
	data.rtt_sum = 0;
	data.rtt_sum_sq = 0;
	data.host = NULL;
}

int resolve_host()
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(data.host, NULL, &hints, &data.res) != 0)
		return(printf("ping: unknown host\n"), 1);

	struct sockaddr_in *dest_addr = (struct sockaddr_in *)data.res->ai_addr;

	if (!inet_ntop(AF_INET, &dest_addr->sin_addr, data.addr_ip, INET_ADDRSTRLEN))
		return (perror("inet_ntop"), freeaddrinfo(data.res), 1);
	return (0);
}

int init_socket()
{
	data.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (data.sockfd == -1) 
		return(perror("socket"), freeaddrinfo(data.res), 1);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (setsockopt(data.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) 
		return(perror("socket"), freeaddrinfo(data.res), 1);
	return (0);
}

void ping_recv(struct timeval tv1, uint16_t sequence)
{
	char recv_packet[PACKET_SIZE]; // Buffer séparé pour la réception
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
	if (recv_bytes > 0)
	{
		if (!inet_ntop(AF_INET, &src_addr.sin_addr, data.addr_ip, INET_ADDRSTRLEN))
		{	perror("inet_ntop"); freeaddrinfo(data.res); exit(1);}
		data.received++;
		// recv_packet: [ IP_HEADER | ICMP_HEADER | ICMP_DATA ] ← paquet reçu complet
		struct iphdr *iphdr = (struct iphdr *)recv_packet; //contient les information ip du packet recu  / ihl = Internet Header Length 
		struct icmphdr *icmphdr = (struct icmphdr *)(recv_packet + iphdr->ihl * 4);
		struct timeval tv2;
		gettimeofday(&tv2, NULL);
		double deltaT = (((tv2.tv_sec - tv1.tv_sec) * 1000.0) + ((tv2.tv_usec - tv1.tv_usec) / 1000.0));
		compute_stat(&data, deltaT);
		//convertie la sequence du reseau vers la langue du pc (big endian -> little)
		printf("%ld bytes from %s: icmp_seq=%d, ttl=%d, time=%.3f ms\n", 
			recv_bytes, data.addr_ip, ntohs(icmphdr->un.echo.sequence), iphdr->ttl, deltaT);
	}
}

void	ping_send_loop()
{
	uint16_t sequence = 0;
	while (!stop)
	{
		data.icmp->un.echo.sequence = htons(sequence);

		gettimeofday((struct timeval *)(data.send_packet + sizeof(struct icmphdr)), NULL);
		struct timeval tv1 = *(struct timeval *)(data.send_packet + sizeof(struct icmphdr));
		icmp_checksum(&data);
	
		ssize_t send = sendto(data.sockfd, data.send_packet, PACKET_SIZE, 0, (struct sockaddr *)data.res->ai_addr, sizeof(struct sockaddr_in));
		if (send == -1)
		{
			perror("sendto");sleep(1);continue;
		}
		data.transmitted++;

		ping_recv(tv1, sequence);
		sequence++;
		if (data.transmitted > 0)
			data.lost = ((data.transmitted - data.received) * 100) / data.transmitted;
		sleep(1);
	}
}

int main(int ac, char **av) {
	int check_v = check_flag(ac, av, &data.host);
	if (check_v != 0 && check_v != 2) return (check_v);
	if (resolve_host() != 0) return 1;
  	if (init_socket() != 0) return 1;

	icmp_builder(&data);
	if (check_v == 0)
        printf("PING %s (%s): %ld data bytes\n", data.host, data.addr_ip, PACKET_SIZE - sizeof(struct icmphdr));
	else if (check_v == 2)
		printf("PING %s (%s): %ld data bytes, id 0x%04x = %d \n", data.host, data.addr_ip, PACKET_SIZE - sizeof(struct icmphdr), data.pid, data.pid);

	init_data();
	signal(SIGINT, handle_sigint);
	ping_send_loop();
	print_stat(&data);
	freeaddrinfo(data.res);
    return (0);
}
