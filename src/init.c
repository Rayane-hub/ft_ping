/* Init helpers: argument parsing, host resolution, socket setup. */

#include "ping.h"

/* Parse CLI flags and extract the host. */
int check_flag(int ac, char **av, char **host) {
	int check_v; // 0 = normal, 2 = verbose

	check_v = 0; // initial value
	for(int i = 1 ; i < ac ; i++)
	{
		if (av[i] && !*host && av[i][0] != '-')
			*host = av[i];
		if (av[i] && av[i][0] == '-')
		{
			if (!*host && av[i][1] == '\0')
				return(printf("ping: unknown host\n"), 1);
			if (av[i][1] == '?'){
				printf("Usage: ft_ping [-v] destination\n-v    verbose output\n-?    display this help\n");
				exit(0);
			}
				else if (av[i][1] == 'v')
				check_v = 2; // verbose mode
			else if (av[i][1] != '\0')
				return(printf("ping: invalid option -- '%c'\nTry 'ping --help' or 'ping --usage' for more information.\n", av[i][1]), 64);
		}
	}
	if (*host == NULL)
		return(printf("ping: missing host operand\nTry 'ping --help' or 'ping --usage' for more information.\n"), 64); 
	return (check_v);
}

/* Initialize runtime data before starting the ping loop. */
void init_data(PingData *data)
{
	data->received = 0;
	data->transmitted = 0;
	data->rtt_max = 0;
	data->rtt_min = DBL_MAX;
	data->rtt_sum = 0;
	data->rtt_sum_sq = 0;
	data->host = NULL;
}

/* Resolve hostname and store the target IPv4 string. */
int resolve_host(PingData *data)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(data->host, NULL, &hints, &data->res) != 0)
		return(printf("ping: unknown host\n"), 1);

	struct sockaddr_in *dest_addr = (struct sockaddr_in *)data->res->ai_addr;

	if (!inet_ntop(AF_INET, &dest_addr->sin_addr, data->addr_ip, INET_ADDRSTRLEN))
		return (perror("inet_ntop"), freeaddrinfo(data->res), 1);
	return (0);
}

/* Create raw socket and configure receive timeout. */
int init_socket(PingData *data)
{
	data->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (data->sockfd == -1) 
		return(perror("socket"), freeaddrinfo(data->res), 1);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (setsockopt(data->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) 
		return(perror("setsockopt"), freeaddrinfo(data->res), 1);
	return (0);
}
