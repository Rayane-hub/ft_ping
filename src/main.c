#include <unistd.h>        // close
#include <errno.h>         // errno
#include <sys/types.h>    // types utilisés par le système (socklen_t, etc.)
#include <sys/socket.h>   // socket()
#include <netdb.h>        // getaddrinfo(), freeaddrinfo()
#include <netinet/in.h>    // IPPROTO_ICMP

#include <stdio.h>        // printf
#include <string.h>       // memset, strcmp

#include <arpa/inet.h>   // inet_ntop, htons

#include <stdint.h>       // uint8_t, uint16_t
#include <sys/time.h>     // gettimeofday, struct timeval
#include <stdlib.h>       // exit

/*
 * === ICMP header definition
 * Nous définissons une petite structure représentant
 * l'en-tête ICMP Echo (type 8) : type, code, checksum, id, seq
 */
struct icmp_hdr {
    uint8_t  type;      // ICMP message type
    uint8_t  code;      // ICMP message code
    uint16_t checksum;  // checksum sur l'entête + payload
    uint16_t id;        // identifiant (souvent le pid)
    uint16_t seq;       // numéro de séquence
};

/*
 * Internet checksum (RFC 1071) pour l'ICMP
 * Parcourt le buffer en mots de 16 bits, ajoute les retenues,
 * et retourne le complément à un.
 */
static uint16_t icmp_checksum(const void *buf, int len)
{
    const uint16_t *data = buf;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *data++;
        len -= 2;
    }
    if (len == 1) {
        // traiter l'octet restant
        sum += *(const uint8_t *)data;
    }
    // ajouter les reports
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

/*
 * Construit un paquet ICMP Echo Request dans le buffer fourni.
 * On place un struct timeval juste après l'en-tête ICMP pour
 * permettre la mesure du RTT (timestamp envoyé au destinataire).
 * Retourne la taille du paquet construit ou -1 en cas d'erreur.
 */
static int build_icmp_echo(char *buf, int buflen, uint16_t id, uint16_t seq)
{
    int hdr_len = sizeof(struct icmp_hdr);
    int payload_len = sizeof(struct timeval); // stocker la date d'envoi
    if (buflen < hdr_len + payload_len)
        return -1; // pas assez d'espace

    struct icmp_hdr *icmp = (struct icmp_hdr *)buf;

    // Remplissage de l'en-tête ICMP
    icmp->type = 8;    // ICMP Echo Request
    icmp->code = 0;
    icmp->checksum = 0; // initialement 0 pour calculer la checksum
    icmp->id = htons(id);
    icmp->seq = htons(seq);

    // Payload : timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    memcpy(buf + hdr_len, &tv, payload_len);

    int pktlen = hdr_len + payload_len;

    // Calcul de la checksum sur l'en-tête ICMP + payload
    icmp->checksum = icmp_checksum(buf, pktlen);

    return pktlen;
}

/*
 * Crée et configure une socket RAW pour ICMP.
 * - définit un timeout de réception (SO_RCVTIMEO)
 * - optionnellement fixe le TTL (IP_TTL)
 * Retourne le descripteur de socket ou -1 en cas d'erreur.
 */
static int create_raw_socket(int ttl)
{
    // Création de la socket RAW ICMP
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    // Timeout de réception : 1 seconde (modifiable)
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        close(sock);
        return -1;
    }

    // (Optionnel) régler le TTL si fourni (> 0)
    if (ttl > 0) {
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
            // On n'échoue pas si la configuration du TTL échoue,
            // mais on affiche une alerte pour le débogage.
            perror("setsockopt IP_TTL");
        }
    }

    return sock;
}

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

	/* Création d'une socket RAW ICMP et configuration basique */
	int ttl = 64; // valeur par défaut du TTL
	int sock = create_raw_socket(ttl);
	if (sock < 0) {
	    // Si la création de la socket RAW échoue, afficher un message clair.
	    // Rappel : une socket RAW requiert des privilèges root ou CAP_NET_RAW.
	    fprintf(stderr, "Error: cannot create raw socket. Are you root?\n");
	    freeaddrinfo(res);
	    return 1;
	}

	// Exemple : construction d'un paquet ICMP Echo Request
	char sendbuf[1500];
	uint16_t id = (uint16_t)getpid();
	uint16_t seq = 1; // pour un envoi unique d'exemple
	int pktlen = build_icmp_echo(sendbuf, sizeof(sendbuf), id, seq);
	if (pktlen < 0) {
	    fprintf(stderr, "Failed to build ICMP packet\n");
	    close(sock);
	    freeaddrinfo(res);
	    return 1;
	}

	// Envoi du paquet (exemple). Cet envoi nécessite les droits root.
	// Si vous ne souhaitez pas envoyer maintenant, commentez les lignes sendto() ci-dessous.
	if (sendto(sock, sendbuf, pktlen, 0, res->ai_addr, res->ai_addrlen) < 0) {
	    perror("sendto");
	    // On continue pour laisser l'utilisateur voir le message d'erreur,
	    // mais on nettoie proprement ensuite.
	} else {
	    printf("ICMP Echo Request envoyé (id=%u seq=%u)\n", id, seq);
	}

	/* getaddrinfo alloue de la mémoire → on libère */

	if (check_v == 0)
                printf("mode normal\n");
        else if (check_v == 2)
                printf("mode verbose\n");

	/* Si on arrive ici: une adresse IPv4 a été trouvée */
	freeaddrinfo(res);

	// fermer la socket RAW
	close(sock);


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
