#include "ping.h"

volatile sig_atomic_t stop = 0;

void handle_sigint()
{
	stop = 1;
}

void ping_recv(struct timeval tv1, uint16_t sequence, PingData *data)
{
	char recv_packet[PACKET_SIZE]; // Buffer séparé pour la réception
	struct sockaddr_in src_addr;
	socklen_t addrlen = sizeof(src_addr);
	ssize_t recv_bytes = recvfrom(data->sockfd, recv_packet, PACKET_SIZE, 0, (struct sockaddr *)&src_addr, &addrlen);
	if (recv_bytes == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			printf("Request timeout for icmp_seq %d\n", sequence);
		else
			perror("recv:");
	}
	if (recv_bytes > 0)
	{
		if (!inet_ntop(AF_INET, &src_addr.sin_addr, data->addr_ip, INET_ADDRSTRLEN))
		{	perror("inet_ntop"); freeaddrinfo(data->res); exit(1);}
		data->received++;
		// recv_packet: [ IP_HEADER | ICMP_HEADER | ICMP_DATA ] ← paquet reçu complet
		struct iphdr *iphdr = (struct iphdr *)recv_packet; //contient les information ip du packet recu  / ihl = Internet Header Length 
		struct icmphdr *icmphdr = (struct icmphdr *)(recv_packet + iphdr->ihl * 4);
		struct timeval tv2;
		gettimeofday(&tv2, NULL);
		double deltaT = (((tv2.tv_sec - tv1.tv_sec) * 1000.0) + ((tv2.tv_usec - tv1.tv_usec) / 1000.0));
		compute_stat(data, deltaT);
		//convertie la sequence du reseau vers la langue du pc (big endian -> little)
		printf("%ld bytes from %s: icmp_seq=%d, ttl=%d, time=%.3f ms\n", 
			recv_bytes, data->addr_ip, ntohs(icmphdr->un.echo.sequence), iphdr->ttl, deltaT);
	}
}

void	ping_send_loop(PingData *data)
{
	uint16_t sequence = 0;
	while (!stop)
	{
		data->icmp->un.echo.sequence = htons(sequence);

		gettimeofday((struct timeval *)(data->send_packet + sizeof(struct icmphdr)), NULL);
		struct timeval tv1 = *(struct timeval *)(data->send_packet + sizeof(struct icmphdr));
		icmp_checksum(data);
	
		ssize_t send = sendto(data->sockfd, data->send_packet, PACKET_SIZE, 0, (struct sockaddr *)data->res->ai_addr, sizeof(struct sockaddr_in));
		if (send == -1)
		{
			perror("sendto");sleep(1);continue;
		}
		data->transmitted++;

		ping_recv(tv1, sequence, data);
		sequence++;
		if (data->transmitted > 0)
			data->lost = ((data->transmitted - data->received) * 100) / data->transmitted;
		sleep(1);
	}
}