/* Ping loop: send requests, receive replies, update stats. */

#include "ping.h"

volatile sig_atomic_t stop = 0;

/* SIGINT handler to stop the main loop. */
void handle_sigint()
{
	stop = 1;
}

/* Validate an ICMP Echo Reply against size, id, type, and sequence. */
bool is_valid_reply(PingData *data, struct icmphdr *icmphdr, ssize_t recv_bytes)
{
	if (recv_bytes <= 0)
    	return false;
	if (recv_bytes < (ssize_t)(sizeof(struct iphdr) + PACKET_SIZE)){
		printf("packet too short: %zd bytes\n", recv_bytes);
		return false;
	}
	if (data->pid != ntohs(icmphdr->un.echo.id)){
		printf("wrong id: got %u expected %u\n",ntohs(icmphdr->un.echo.id), data->pid);
		return false;
	}
	if (icmphdr->type != ICMP_ECHOREPLY || icmphdr->code != 0){
   		printf("not echo reply: type=%d code=%d\n", icmphdr->type, icmphdr->code);
    	return false;
	}
	if (ntohs(data->icmp->un.echo.sequence) != ntohs(icmphdr->un.echo.sequence)){
		printf("wrong seq recv");
		return false;
	}
	return true;
}

/* Receive a packet, validate it, and update RTT stats. */
void ping_recv(struct timeval tv1, uint16_t sequence, PingData *data)
{
	char recv_packet[RECV_BUF_SIZE];
	struct sockaddr_in src_addr;
	socklen_t addrlen = sizeof(src_addr);
	ssize_t recv_bytes = recvfrom(data->sockfd, recv_packet, RECV_BUF_SIZE, 0, (struct sockaddr *)&src_addr, &addrlen);
	if (recv_bytes == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			printf("Request timeout for icmp_seq %d\n", sequence);
		else
			perror("recv:");
		return;
	}
	// recv_packet: [ IP_HEADER | ICMP_HEADER | ICMP_DATA ]
	if (recv_bytes < (ssize_t)sizeof(struct iphdr))
		return;
	struct iphdr *iphdr = (struct iphdr *)recv_packet;
	if (recv_bytes < (ssize_t)sizeof(struct icmphdr) + iphdr->ihl * 4)
		return;
	struct icmphdr *icmphdr = (struct icmphdr *)(recv_packet + iphdr->ihl * 4);
	if (!is_valid_reply(data, icmphdr, recv_bytes))
		return;
	if (!inet_ntop(AF_INET, &src_addr.sin_addr, data->addr_ip, INET_ADDRSTRLEN))
	{	perror("inet_ntop"); freeaddrinfo(data->res); exit(1);	}
	data->received++;
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	double deltaT = (((tv2.tv_sec - tv1.tv_sec) * 1000.0) + ((tv2.tv_usec - tv1.tv_usec) / 1000.0));
	compute_stat(data, deltaT);
	printf("%ld bytes from %s: icmp_seq=%d, ttl=%d, time=%.3f ms\n", 
		recv_bytes, data->addr_ip, ntohs(icmphdr->un.echo.sequence), iphdr->ttl, deltaT);
	
}

/* Main send/receive loop. */
void ping_send_loop(PingData *data)
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
		{	perror("sendto");sleep(1);continue;	}
		data->transmitted++;

		ping_recv(tv1, sequence, data);
		sequence++;
		if (data->transmitted > 0)
			data->lost = ((data->transmitted - data->received) * 100) / data->transmitted;
		sleep(1);
	}
}
