#include <unistd.h>        // close
#include <errno.h>         // errno
#include <sys/types.h>    // types utilisés par le système (socklen_t, etc.)
#include <sys/socket.h>   // socket()
#include <netdb.h>        // getaddrinfo(), freeaddrinfo()
#include <netinet/in.h>    // IPPROTO_ICMP

#include <stdio.h>        // printf
#include <string.h>       // memset, strcmp

#include <arpa/inet.h>   // inet_ntop

void ft_print_adrr(struct addrinfo *res) {
	char ip_str[INET_ADDRSTRLEN]; // buffer pour l'IP en texte

	struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr; // caster vers IPv4

	inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str)); // convertir binaire -> chaîne

	printf("Resolved IP = %s\n", ip_str); // afficher l'adresse résolue

}

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
			if (av[i][1] == '?')
				return(printf("Usage: ft_ping [-v] destination\n-v    verbose output\n-?    display this help\n"), 0); // aide
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

int main(int ac, char **av) {
	int check_v;
    char *host = NULL;

	check_v = ft_flag(ac, av, &host);
	printf("flag = %d\nhost = |%s|\n", check_v, host);
	if (check_v != 0 && check_v != 2)
			return (check_v);

	if (check_v == 0)
                printf("mode normal\n");
	else if (check_v == 2)
			printf("mode verbose\n");

        return (0);
}
