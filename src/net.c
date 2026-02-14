/* `net.c` gère la préparation des paquets ICMP (entête + checksum) avant envoi. */

#include "ping.h"

/* Module réseau ICMP:
 * - prépare le paquet ICMP Echo Request
 * - calcule le checksum ICMP avant envoi
 */
void icmp_builder(PingData *data)
{
	/* Initialise tout le buffer d'envoi à 0 pour éviter des données résiduelles. */
	memset(data->send_packet, 0, PACKET_SIZE);
	
	/* Place l'en-tête ICMP au début du paquet et renseigne les champs fixes. */
	data->icmp = (struct icmphdr *)data->send_packet;
	data->icmp->type = ICMP_ECHO; // 8
	data->icmp->code = 0;

	/* Utilise le PID du processus comme identifiant ICMP (sur 16 bits). */
	data->pid = getpid() & 0xFFFF;
	data->icmp->un.echo.id = htons(data->pid); //convertie le pid du pc vers la langue du reseau (little -> big endian)
}

void icmp_checksum(PingData *data)
{
	/* Recalcule le checksum ICMP sur l'ensemble du paquet à envoyer. */
	data->icmp->checksum = 0;
	uint32_t sum = 0;
	uint16_t *ptr = (uint16_t *)data->send_packet;
	size_t icmp_len = PACKET_SIZE;

	/* Addition mot par mot (16 bits), puis repli des retenues. */
	for (size_t i = 0; i < icmp_len / 2; i++)
		sum += ptr[i];
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (sum & 0xFFFF) + (sum >> 16);

	/* Complément à 1 final du checksum. */
	data->icmp->checksum = ~((uint16_t)sum);
}
