#include <sys/socket.h>	//struct sockaddr
#include <netdb.h>		// getaddrinfo(), freeaddrinfo()
#include <stdio.h>		// printf()
#include <string.h>		// memset(), strcmp()
#include <arpa/inet.h> 	//inet_ntop(),htons()
#include <stdlib.h>		//exit()
#include <netinet/ip_icmp.h> // struct icmphdr
#include <unistd.h> 	// getpid()

#define INET_ADDRSTRLEN 16
#define PACKET_SIZE 64

typedef struct ping_data {
	struct addrinfo *res;
	int sockfd;
	char packet[PACKET_SIZE];
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

void ft_print_ip(struct sockaddr_in *addr, char *host){
	char buffer[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr->sin_addr, buffer, INET_ADDRSTRLEN);
	printf("\nIP de %s = %s\n\n", host, buffer);
}

void	ft_icmp_builder(PingData *data)
{
	memset(data->packet, 0, PACKET_SIZE);


	data->icmp = (struct icmphdr *)data->packet;
	data->icmp->type = 8;
	data->icmp->code = 0;
	data->icmp->checksum = 0;
	data->icmp->un.echo.id = htons(getpid() & 0xFFFF);
	printf("id = %d\n", data->icmp->un.echo.id);
	data->icmp->un.echo.sequence = 0;
	
	//calcul checksum
	uint32_t sum = 0;
	uint16_t *ptr = (uint16_t *)data->packet;
	for (size_t i = 0; i < PACKET_SIZE / 2; i++)
		sum += ptr[i];
	sum = (sum & 0xFFFF) + (sum >> 16);
	data->icmp->checksum = (uint16_t)sum;
	printf("checksum = %d\n", data->icmp->checksum);
}

int main(int ac, char **av) {
	int check_v;
	char *host = NULL;
	PingData data;

	check_v = ft_flag(ac, av, &host);
	printf("flag = %d\thost = |%s|\n", check_v, host);
	if (check_v != 0 && check_v != 2)
		return (check_v);

	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(host, NULL, &hints, &data.res) != 0)
		return(printf("ping: unknown host\n"), 1);

	struct sockaddr_in *dest_addr = (struct sockaddr_in *)data.res->ai_addr;
	ft_print_ip(dest_addr, host);
	
	data.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (data.sockfd == -1)
		return(printf("err socket\n"), 1);

	ft_icmp_builder(&data);

	ssize_t send = sendto(data.sockfd, data.packet, PACKET_SIZE, 0, (struct sockaddr *)data.res->ai_addr, sizeof(struct sockaddr_in));
	if (send == -1)
    	perror("sendto");
	
	if (check_v == 0)
                printf("mode normal\n");
	else if (check_v == 2)
			printf("mode verbose\n");
	freeaddrinfo(data.res);
    return (0);
}
