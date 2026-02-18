/* Prepare ICMP packets before sending. */

#include "ping.h"

/* Build the ICMP Echo Request header (type, code, id). */
void icmp_builder(PingData *data)
{
	memset(data->send_packet, 0, PACKET_SIZE);
	
	data->icmp = (struct icmphdr *)data->send_packet;
	data->icmp->type = ICMP_ECHO; // 8
	data->icmp->code = 0;

	data->pid = getpid() & 0xFFFF;
	data->icmp->un.echo.id = htons(data->pid);
}

/* Compute the ICMP checksum before sending. */
void icmp_checksum(PingData *data)
{
	data->icmp->checksum = 0;
	uint32_t sum = 0;
	uint16_t *ptr = (uint16_t *)data->send_packet;
	size_t icmp_len = PACKET_SIZE;

	for (size_t i = 0; i < icmp_len / 2; i++)
		sum += ptr[i];
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (sum & 0xFFFF) + (sum >> 16);

	data->icmp->checksum = ~((uint16_t)sum);
}
