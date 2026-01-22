#include <unistd.h>        // close
#include <errno.h>         // errno
#include <sys/types.h>    // types utilisés par le système (socklen_t, etc.)
#include <sys/socket.h>   // socket()
#include <netdb.h>        // getaddrinfo(), freeaddrinfo()
#include <netinet/in.h>    // IPPROTO_ICMP

#include <stdio.h>        // printf
#include <string.h>       // memset, strcmp

#include <arpa/inet.h>   // inet_ntop

void ft_print_adrr(struct addrinfo *res)
{
	char ip_str[INET_ADDRSTRLEN];

	struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;

	inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));

	printf("Resolved IP = %s\n", ip_str);

}

int ft_flag(int ac, char **av, char **host)
{
        int check_v = 0;
	for(int i = 1 ; i < ac ; i++)
	{
                if (av[i] && !*host && av[i][0] != '-')
                        *host = av[i];
		if (av[i] && av[i][0] == '-')
                {
                        if (!*host && av[i][1] == '\0')
                                return(printf("ping: unknown host\n"), 1);
			if (av[i][1] == '?')
	                       return(printf("Usage: ft_ping [-v] destination\n-v    verbose output\n-?    display this help\n"), 0);
       			else if (av[i][1] == 'v')
                                 check_v = 2;
       			else if (av[i][1] != '\0')
                                return(printf("ping: invalid option -- '%c'\nTry 'ping --help' or 'ping --usage' for more information.\n", av[i][1]), 64);
                }
	}
        if (!*host)
                return(printf("ping: missing host operand\nTry 'ping --help' or 'ping --usage' for more information.\n"), 64);
        return (check_v);
}

int main(int ac, char **av)
{
        char *host = NULL;
	int check_v = ft_flag(ac, av, &host);
        printf("flag = %d\nhost = |%s|\n", check_v, host);
        if (check_v != 0 && check_v != 2)
                return (check_v);

	/* =========================
	   RESOLUTION HOST -> IP
	   ========================= */

	struct addrinfo hints;   // filtres: ce que l’on demande à l’OS
	struct addrinfo *res;    // résultat: adresse(s) trouvée(s)

	/* On met toute la structure à zéro pour éviter
	   des valeurs aléatoires dans les champs */
	memset(&hints, 0, sizeof(hints));

	/* On demande uniquement de l’IPv4 (AF_INET) */
	hints.ai_family = AF_INET;

	/* Appel système: transforme "google.com" ou "8.8.8.8" en adresse réseau */
	int ret = getaddrinfo(host, NULL, &hints, &res);
	/* Si DNS / résolution échoue */
	if (ret != 0)
	    return(printf("ping: unknown host\n"), 64);
	ft_print_adrr(res);

	printf("Host resolved successfully\n");

	/* getaddrinfo alloue de la mémoire → on libère */

	if (check_v == 0)
                printf("mode normal\n");
        else if (check_v == 2)
                printf("mode verbose\n");

	/* Si on arrive ici: une adresse IPv4 a été trouvée */
	freeaddrinfo(res);


        return (0);
}


/*
./ft_ping
./ft_ping -?
./ft_ping 8.8.8.8
./ft_ping -v 8.8.8.8
./ft_ping -x
*/
/*
string host
   ↓
getaddrinfo()
   ↓
adresse réseau valide (struct sockaddr)
*/
