#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <arpa/inet.h>

unsigned int rtable_len;
unsigned int arp_table_len;
struct route_table_entry *rtable = NULL;
struct arp_table_entry *arp_table = NULL;


int check_mac(u_int8_t* my_address, u_int8_t* recieved_address) {
	int ok = 1;
	
	for (int i = 0; i < MAC_SIZE && ok; i ++) {
		if (my_address[i] != recieved_address[i]) {
			ok = 0;
		}
	}
	if (ok == 1) {
		return ok;
	} else {
		ok = 1; 
		for (int i = 0; i < MAC_SIZE; i ++) {
			if (recieved_address[i] != 0xff) {
				ok = 0;
				break;
			}
		}
		return ok;
	}

}

struct route_table_entry* find_best_route(uint32_t ip_dest) {
	struct route_table_entry *best = NULL;
	for (int i = 0; i < rtable_len; i ++) {
		if ((ip_dest & rtable[i].mask) == rtable[i].prefix) {
      		if (best == NULL) {
        	best = &rtable[i];
			} else if (ntohl(best->mask) < ntohl(rtable[i].mask)) {
				best = &rtable[i];
			}
    	}
	}
	return best;
}

struct arp_table_entry* get_mac_address(uint32_t next_hop) {
	for (int i = 0; i < arp_table_len; i ++) {
		if (next_hop == arp_table[i].ip) {
			return &arp_table[i];
		}
	}
	return NULL;
}


int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	int rc;
	// TODO route_table
	
	rtable = (struct route_table_entry *) malloc(MAX_ENTRIES * sizeof(struct route_table_entry));
	DIE(rtable == NULL, "memory");

	rtable_len = read_rtable(argv[1], rtable);

	// initialize arp_table as empty
	arp_table = (struct arp_table_entry *) malloc(MAX_ENTRIES_MAC * sizeof(struct arp_table_entry));
	DIE(arp_table == NULL, "memory");
	arp_table_len = 0;

	// initialize queue for waiting after arp
	queue waiting_queue = queue_create();
	
	// allocate memory for mac address
	uint8_t *mac_address = malloc(MAC_SIZE * sizeof(u_int8_t));
	DIE(mac_address == NULL, "memory");

	while (1) {

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */

		// USEFUL REMEBERS: ntohs() -> transform from little endian order to network order
		// 					htons() -> transform from network order to little endian order
		// fprintf(stderr, "Packet recieved from: ");
		// for (int i = 0; i < MAC_SIZE; i ++) {
		// 	fprintf(stderr, "%x ", eth_hdr->ether_shost[i]);
		// }
		// fprintf(stderr, "for: ");
		// for (int i = 0; i < MAC_SIZE; i ++) {
		// 	fprintf(stderr, "%x ", eth_hdr->ether_dhost[i]);
		// }
		// check lenght of packet??

		// check destination
		get_interface_mac(interface, mac_address);

		if (check_mac(mac_address, eth_hdr->ether_dhost) == 1) {
			// check type
			if (ntohs(eth_hdr->ether_type) == 0x0800) {
				fprintf(stderr, "Recived IPv4\n");
				// initialize IP header
				struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header)); 
				
				// check if destintation
				uint8_t* recieved_ip = (uint8_t *)&ip_hdr->daddr;
				char *router_ip_char = get_interface_ip(interface);
				uint32_t router_ip = 0;
				int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
				DIE(rc < 0, "inet");
				if (ip_hdr->daddr == router_ip) {
					// ICMP PROTOCOL
					// init icmp header
					struct icmphdr* icmp_hdr = (struct icmphdr *)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));
					// check checksum
					uint16_t old_checksum = icmp_hdr->checksum;
					icmp_hdr->checksum = 0;

					if (old_checksum != ntohs(checksum((u_int16_t *)icmp_hdr,
						 len - sizeof(struct ether_header) - sizeof(struct iphdr)))) {
						// fprintf(stderr, "CORRPUTED PACKET -- DROP\n");
      					continue;	
					}
					// check if it is a request
					if (icmp_hdr->type == 8 && icmp_hdr->code == 0) {
						// send reply
						icmp_hdr->type = 0;
						icmp_hdr->checksum = htons(checksum((u_int16_t *)icmp_hdr,
						 	len - sizeof(struct ether_header) - sizeof(struct iphdr)));
						
						// change ip daddr saddr
						uint32_t aux = ip_hdr->daddr;
						ip_hdr->daddr = ip_hdr->saddr;
						ip_hdr->saddr = aux;

						// search next hop
						struct route_table_entry *best_route = find_best_route(ip_hdr->daddr);

						struct arp_table_entry *next_hop = get_mac_address(best_route->next_hop);
						if (next_hop == NULL) {
							
							// ************************
							fprintf(stderr, "SEND ARP REQUEST\n");
							// create ARP
							char * packet = malloc(sizeof(struct ether_header) + sizeof(struct arp_header));
							DIE(packet == NULL, "memory");
							struct ether_header *ether = (struct ether_header *)packet;
							struct arp_header *arp = (struct arp_header *) (packet + sizeof(struct ether_header));

							// set dhost to broadcast
							// set target mac
							for (int i = 0; i <MAC_SIZE; i ++) {
								ether->ether_dhost[i] = 0xff;
								arp->tha[i] = 0x00;
							}

							// set ether type to arp
							ether->ether_type = ntohs(0x0806);

							// initialize arp
							arp->htype = ntohs(0x01); 
							arp->ptype = ntohs(0x0800); 
							arp->hlen = 0x06;
							arp->plen = 0x04;
							arp->op = ntohs(0x01);

							// update mac router and ip router
							// for correct interface
							get_interface_mac(best_route->interface, mac_address);
							char *router_ip_char = get_interface_ip(interface);
							uint32_t router_ip = 0;
							int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
							DIE(rc < 0, "inet");


							// copy sender mac
							// set shost to router mac					
							memcpy(arp->sha, mac_address, sizeof(arp->sha));
							memcpy(ether->ether_shost, mac_address,
								sizeof(ether->ether_shost));
							
							// set ip addresses
							arp->spa = router_ip;
							arp->tpa = best_route->next_hop;

							// add packet to watining_queue
							struct packet *current_packet = malloc(sizeof(struct packet));
							current_packet->len = len;
							current_packet->buf = malloc(len);
							memcpy(current_packet->buf, buf, len);
							queue_enq(waiting_queue, (void *) current_packet);
							
							// send ARP
							send_to_link(best_route->interface, packet, sizeof(struct ether_header) + sizeof(struct arp_header));
							
							// free memory
							free(packet);
							continue;
							// *******************
						}

						memcpy(eth_hdr->ether_dhost, next_hop->mac,
							sizeof(eth_hdr->ether_dhost));
						memcpy(eth_hdr->ether_shost, mac_address,
							sizeof(eth_hdr->ether_shost));
						// send packet
						send_to_link(best_route->interface, buf, len);
					}

				} else {
					// check checksum
					uint16_t old_check = ip_hdr->check;
					ip_hdr->check = 0;
					if (old_check !=
						ntohs(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)))) {
							// fprintf(stderr, "CORRPUTED PACKET -- DROP\n");
							continue;
					}

					// check TTL
					if (ip_hdr->ttl <= 1) {
						fprintf(stderr, "TTL expired -- DROP\n");
						// TODO send ICMP
						// *******************
						// create buffer
						size_t buffer_len = sizeof(struct ether_header) + 2 * sizeof(struct iphdr) 
							+ sizeof(struct icmphdr) + sizeof(uint64_t);
						

						// init icmp_header
						struct icmphdr *icmp_header = (struct icmphdr *)(buf + sizeof(struct ether_header) 
							+ sizeof(struct iphdr));
						
						// copy original ip header and 64 bits
						char *copy_dest = buf + sizeof(struct ether_header) 
							+ sizeof(struct iphdr) + sizeof(struct icmphdr);
						memcpy(copy_dest, ip_hdr, sizeof(struct iphdr) + sizeof(uint64_t));

						// fill icmp header
						icmp_header->type = 0xb;
						icmp_header->code = 0x0;
						icmp_header->un.echo.id = 0;
						icmp_header->un.echo.sequence = 0; 
						icmp_header->checksum = 0;
						icmp_header->checksum = htons(checksum((u_int16_t *)icmp_header,
							buffer_len - sizeof(struct ether_header) - sizeof(struct iphdr)));

						// update ip header
						ip_hdr->ttl = 64;
						ip_hdr->check = 0;
						ip_hdr->protocol = 1;
						ip_hdr->tot_len = htons(buffer_len - sizeof(struct ether_header));
						ip_hdr->daddr = ip_hdr->saddr;
						ip_hdr->saddr = router_ip;
						ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

						// update eth header
						struct route_table_entry *best_route = find_best_route(ip_hdr->daddr);
						if (best_route == NULL) {
							continue;
						}
						struct arp_table_entry *next_hop = get_mac_address(best_route->next_hop);
						if (next_hop == NULL) {
							
							// ************************
							fprintf(stderr, "SEND ARP REQUEST\n");
							// create ARP
							char * packet = malloc(sizeof(struct ether_header) + sizeof(struct arp_header));
							DIE(packet == NULL, "memory");
							struct ether_header *ether = (struct ether_header *)packet;
							struct arp_header *arp = (struct arp_header *) (packet + sizeof(struct ether_header));

							// set dhost to broadcast
							// set target mac
							for (int i = 0; i <MAC_SIZE; i ++) {
								ether->ether_dhost[i] = 0xff;
								arp->tha[i] = 0x00;
							}

							// set ether type to arp
							ether->ether_type = ntohs(0x0806);

							// initialize arp
							arp->htype = ntohs(0x01); 
							arp->ptype = ntohs(0x0800); 
							arp->hlen = 0x06;
							arp->plen = 0x04;
							arp->op = ntohs(0x01);

							// update mac router and ip router
							// for correct interface
							get_interface_mac(best_route->interface, mac_address);
							char *router_ip_char = get_interface_ip(interface);
							uint32_t router_ip = 0;
							int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
							DIE(rc < 0, "inet");


							// copy sender mac
							// set shost to router mac					
							memcpy(arp->sha, mac_address, sizeof(arp->sha));
							memcpy(ether->ether_shost, mac_address,
								sizeof(ether->ether_shost));
							
							// set ip addresses
							arp->spa = router_ip;
							arp->tpa = best_route->next_hop;

							// add packet to watining_queue
							struct packet *current_packet = malloc(sizeof(struct packet));
							current_packet->len = buffer_len;
							current_packet->buf = malloc(buffer_len);
							memcpy(current_packet->buf, buf, buffer_len);
							queue_enq(waiting_queue, (void *) current_packet);
							
							// send ARP
							send_to_link(best_route->interface, packet, sizeof(struct ether_header) + sizeof(struct arp_header));
							
							// free memory
							free(packet);
							continue;
							// *******************
						}

						// rewrite L2 address
						memcpy(eth_hdr->ether_shost, mac_address,
							sizeof(eth_hdr->ether_shost));
						memcpy(eth_hdr->ether_dhost, next_hop->mac,
							sizeof(eth_hdr->ether_dhost));
						
						// send packet
						fprintf(stderr, "INTERFACE: %d\n", best_route->interface);
						send_to_link(best_route->interface, buf, buffer_len);
						// *****************
						continue;
					}

					// update TTL
					uint8_t old_ttl = ip_hdr->ttl;
					ip_hdr->ttl -= 1;

					// search route table
					struct route_table_entry *best_route = find_best_route(ip_hdr->daddr);
					if (best_route == NULL) {
						// fprintf(stderr, "PACKET DROP\n");
						// send ICMP
						// *******************
						// create buffer
						size_t buffer_len = sizeof(struct ether_header) + 2 * sizeof(struct iphdr) 
							+ sizeof(struct icmphdr) + sizeof(uint64_t);
						

						// init icmp_header
						struct icmphdr *icmp_header = (struct icmphdr *)(buf + sizeof(struct ether_header) 
							+ sizeof(struct iphdr));
						
						// copy original ip header and 64 bits
						char *copy_dest = buf + sizeof(struct ether_header) 
							+ sizeof(struct iphdr) + sizeof(struct icmphdr);
						memcpy(copy_dest, ip_hdr, sizeof(struct iphdr) + sizeof(uint64_t));

						// fill icmp header
						icmp_header->type = 0x3;
						icmp_header->code = 0x0;
						icmp_header->un.echo.id = 0;
						icmp_header->un.echo.sequence = 0; 
						icmp_header->checksum = 0;
						icmp_header->checksum = htons(checksum((u_int16_t *)icmp_header,
							buffer_len - sizeof(struct ether_header) - sizeof(struct iphdr)));

						// update ip header
						ip_hdr->ttl = 64;
						ip_hdr->check = 0;
						ip_hdr->protocol = 1;
						ip_hdr->tot_len = htons(buffer_len - sizeof(struct ether_header));
						ip_hdr->daddr = ip_hdr->saddr;
						ip_hdr->saddr = router_ip;
						ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

						// update eth header
						struct route_table_entry *best_route = find_best_route(ip_hdr->daddr);
						if (best_route == NULL) {
							continue;
						}
						struct arp_table_entry *next_hop = get_mac_address(best_route->next_hop);
						if (next_hop == NULL) {
							
							// ************************
							fprintf(stderr, "SEND ARP REQUEST\n");
							// create ARP
							char * packet = malloc(sizeof(struct ether_header) + sizeof(struct arp_header));
							DIE(packet == NULL, "memory");
							struct ether_header *ether = (struct ether_header *)packet;
							struct arp_header *arp = (struct arp_header *) (packet + sizeof(struct ether_header));

							// set dhost to broadcast
							// set target mac
							for (int i = 0; i <MAC_SIZE; i ++) {
								ether->ether_dhost[i] = 0xff;
								arp->tha[i] = 0x00;
							}

							// set ether type to arp
							ether->ether_type = ntohs(0x0806);

							// initialize arp
							arp->htype = ntohs(0x01); 
							arp->ptype = ntohs(0x0800); 
							arp->hlen = 0x06;
							arp->plen = 0x04;
							arp->op = ntohs(0x01);

							// update mac router and ip router
							// for correct interface
							get_interface_mac(best_route->interface, mac_address);
							char *router_ip_char = get_interface_ip(interface);
							uint32_t router_ip = 0;
							int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
							DIE(rc < 0, "inet");


							// copy sender mac
							// set shost to router mac					
							memcpy(arp->sha, mac_address, sizeof(arp->sha));
							memcpy(ether->ether_shost, mac_address,
								sizeof(ether->ether_shost));
							
							// set ip addresses
							arp->spa = router_ip;
							arp->tpa = best_route->next_hop;

							// add packet to watining_queue
							struct packet *current_packet = malloc(sizeof(struct packet));
							current_packet->len = buffer_len;
							current_packet->buf = malloc(buffer_len);
							memcpy(current_packet->buf, buf, buffer_len);
							queue_enq(waiting_queue, (void *) current_packet);
							
							// send ARP
							send_to_link(best_route->interface, packet, sizeof(struct ether_header) + sizeof(struct arp_header));
							
							// free memory
							free(packet);
							continue;
							// *******************
						}

						// rewrite L2 address
						memcpy(eth_hdr->ether_shost, mac_address,
							sizeof(eth_hdr->ether_shost));
						memcpy(eth_hdr->ether_dhost, next_hop->mac,
							sizeof(eth_hdr->ether_dhost));
						
						// send packet
						fprintf(stderr, "INTERFACE: %d\n", best_route->interface);
						send_to_link(best_route->interface, buf, buffer_len);
						// *****************


						continue;
					}

					// update checksum
					ip_hdr->check =
						~(~old_check + ~((uint16_t)old_ttl) + (uint16_t)ip_hdr->ttl) - 1;

					// get mac address from ip_dest
					struct arp_table_entry *next_hop = get_mac_address(best_route->next_hop);
					if (next_hop == NULL) {
						
						// ************************
						fprintf(stderr, "SEND ARP REQUEST\n");
						// create ARP
						char * packet = malloc(sizeof(struct ether_header) + sizeof(struct arp_header));
						DIE(packet == NULL, "memory");
						struct ether_header *ether = (struct ether_header *)packet;
						struct arp_header *arp = (struct arp_header *) (packet + sizeof(struct ether_header));

						// set dhost to broadcast
						// set target mac
						for (int i = 0; i <MAC_SIZE; i ++) {
							ether->ether_dhost[i] = 0xff;
							arp->tha[i] = 0x00;
						}

						// set ether type to arp
						ether->ether_type = ntohs(0x0806);

						// initialize arp
						arp->htype = ntohs(0x01); 
						arp->ptype = ntohs(0x0800); 
						arp->hlen = 0x06;
						arp->plen = 0x04;
						arp->op = ntohs(0x01);

						// update mac router and ip router
						// for correct interface
						get_interface_mac(best_route->interface, mac_address);
						char *router_ip_char = get_interface_ip(interface);
						uint32_t router_ip = 0;
						int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
						DIE(rc < 0, "inet");


						// copy sender mac
						// set shost to router mac					
						memcpy(arp->sha, mac_address, sizeof(arp->sha));
						memcpy(ether->ether_shost, mac_address,
							sizeof(ether->ether_shost));
						
						// set ip addresses
						arp->spa = router_ip;
						arp->tpa = best_route->next_hop;

						// add packet to watining_queue
						struct packet *current_packet = malloc(sizeof(struct packet));
						current_packet->len = len;
						current_packet->buf = malloc(len);
						memcpy(current_packet->buf, buf, len);
						queue_enq(waiting_queue, (void *) current_packet);
						
						// send ARP
						send_to_link(best_route->interface, packet, sizeof(struct ether_header) + sizeof(struct arp_header));
						
						// free memory
						free(packet);
						continue;
						// *******************
					}

					// rewrite L2 address
					memcpy(eth_hdr->ether_dhost, next_hop->mac,
						sizeof(eth_hdr->ether_dhost));
					
					// send packet
					send_to_link(best_route->interface, buf, len);
				}

			} else if (ntohs(eth_hdr->ether_type) == 0x0806) {
				fprintf(stderr, "ARP  ");
				// initialize arp header
				struct arp_header *arp_hdr = (struct arp_header*)(buf + sizeof(struct ether_header)); 
				
				// check command
				if (htons(arp_hdr->op) == 0x01) {
					// request
					fprintf(stderr, "Recieved REQUEST\n");
					// have to send back a reply
					
					// change op
					arp_hdr->op = ntohs(0x02);
					// get current router ip
					char *router_ip_char = get_interface_ip(interface);
					uint32_t router_ip = 0;
					int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
					DIE(rc < 0, "inet");

					// change target and source
					memcpy(arp_hdr->tha, arp_hdr->sha,MAC_SIZE * sizeof(uint8_t));
					memcpy(arp_hdr->sha, mac_address,MAC_SIZE * sizeof(uint8_t));
					
					arp_hdr->tpa = arp_hdr->spa;
					arp_hdr->spa = router_ip;
					
					// change eth
					memcpy(eth_hdr->ether_dhost, arp_hdr->tha,
						sizeof(eth_hdr->ether_dhost));
					memcpy(eth_hdr->ether_shost, mac_address,
						sizeof(eth_hdr->ether_shost));
				
					// send packet
					send_to_link(interface, buf, len);

					
				} else {
					// reply
					fprintf(stderr, "Recieved REPLY\n");	

					// add to table
					arp_table[arp_table_len].ip = arp_hdr->spa;
					memcpy(arp_table[arp_table_len].mac, arp_hdr->sha, sizeof(arp_hdr->sha));
					arp_table_len += 1;
					// remove from queue and send packet
					struct packet *current_packet = (struct packet *)queue_deq(waiting_queue);
					struct ether_header *ether = (struct ether_header *)current_packet->buf;
					struct iphdr *ip_hdr = (struct iphdr *)(current_packet->buf + sizeof(struct ether_header)); 
					struct route_table_entry *best_route = find_best_route(ip_hdr->daddr);
					struct arp_table_entry *next_hop = get_mac_address(best_route->next_hop);
					
					if (next_hop == NULL) {
						fprintf(stderr, "something bad happen\n");
						uint8_t *ip_bytes = (uint8_t *) &ip_hdr->saddr;
						fprintf(stderr, "ip->sad: %d %d %d %d \n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
						continue;
					}	
					// update mac router
					get_interface_mac(best_route->interface, mac_address);
					
					// rewrite ether header addresses
					memcpy(ether->ether_dhost, next_hop->mac,
						sizeof(eth_hdr->ether_dhost));
		    		memcpy(ether->ether_shost, mac_address,
						sizeof(eth_hdr->ether_shost));
					// send packet
					fprintf(stderr, "INTERFACE: %d\n", best_route->interface);
						
					send_to_link(best_route->interface, current_packet->buf, current_packet->len);
					// free memory
					free(current_packet->buf);
					free(current_packet);
					current_packet = NULL;
				}
			} else {
				fprintf(stderr, "UNKNOWN -- DROP PACKET\n");
				continue;
			}

		} else {
			fprintf(stderr, "Recieved WRONG MAC -> DROP PACKET\n");
			fprintf(stderr, "Packet recieved from: ");
			for (int i = 0; i < MAC_SIZE; i ++) {
				fprintf(stderr, "%x ", eth_hdr->ether_shost[i]);
			}
			fprintf(stderr, "for: ");
			for (int i = 0; i < MAC_SIZE; i ++) {
				fprintf(stderr, "%x ", eth_hdr->ether_dhost[i]);
			}
			fprintf(stderr, ". My mac is: ");
			for (int i = 0; i < MAC_SIZE; i ++) {
				fprintf(stderr, "%x ", mac_address[i]);
			}
			continue;
		}

	}
	// free memory
	free(mac_address);
	free(arp_table);
	free(rtable);
}

