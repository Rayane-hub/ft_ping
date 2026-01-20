#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int ac, char **av)
{
	if (ac <= 1)
		return (printf("ping: missing host operand\nTry 'ping --help' or 'ping --usage' for more information.\n"), 64);
	if(strcmp(av[1], "-?") == 0)
		printf("print -?\n");

}


/*
./ft_ping
./ft_ping -?
./ft_ping 8.8.8.8
./ft_ping -v 8.8.8.8
./ft_ping -x
*/
