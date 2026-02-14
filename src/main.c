#include "ping.h"

int main(int ac, char **av) {
	PingData data;

	init_data(&data);

	int check_v = check_flag(ac, av, &data.host);
	if (check_v != 0 && check_v != 2) return (check_v);
	if (resolve_host(&data) != 0) return 1;
  	if (init_socket(&data) != 0) return 1;

	icmp_builder(&data);
	if (check_v == 0)
        printf("PING %s (%s): %ld data bytes\n", data.host, data.addr_ip, PACKET_SIZE - sizeof(struct icmphdr));
	else if (check_v == 2)
		printf("PING %s (%s): %ld data bytes, id 0x%04x = %d \n", data.host, data.addr_ip, PACKET_SIZE - sizeof(struct icmphdr), data.pid, data.pid);

	signal(SIGINT, handle_sigint);
	ping_send_loop(&data);
	print_stat(&data);
	freeaddrinfo(data.res);
    return (0);
}
