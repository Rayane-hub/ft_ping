#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        if (check_v == 0)
                printf("mode normal\n");
        else if (check_v == 2)
                printf("mode verbose\n");
        else
                return (check_v);
        return (0);
}


/*
./ft_ping
./ft_ping -?
./ft_ping 8.8.8.8
./ft_ping -v 8.8.8.8
./ft_ping -x
*/
