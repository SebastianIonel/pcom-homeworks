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
	// u_int8_t *ip_bytes = (u_int8_t *)&ip_dest;
	// // fprintf(stderr, "%d %d %d %d -- ", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);

	// ip_bytes = (u_int8_t *) &rtable[0].prefix;
	// // fprintf(stderr, "%d %d %d %d -- ", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
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


	// TODO route_table
	
	rtable = (struct route_table_entry *) malloc(MAX_ENTRIES * sizeof(struct route_table_entry));
	DIE(rtable == NULL, "memory");

	rtable_len = read_rtable(argv[1], rtable);

	// TEST RTABLE => OK
	// for (int i = 0; i < 10; i ++) {
	// uint8_t *ip_bytes = (uint8_t *) &rtable[i].prefix;
	// 		// fprintf(stderr, "%d %d %d %d -- ", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
	// 	ip_bytes = (uint8_t *) &rtable[i].next_hop;
	//   	// fprintf(stderr, "%d %d %d %d -- ", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
	// 	ip_bytes = (uint8_t *) &rtable[i].mask;
	//   	// fprintf(stderr, "%d %d %d %d \n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
	// }

	// TODO arp_table

	arp_table = (struct arp_table_entry *) malloc(MAX_ENTRIES * sizeof(struct arp_table_entry));
	DIE(arp_table == NULL, "memory");

	arp_table_len = parse_arp_table("arp_table.txt", arp_table);

	// TEST ARP => OK
	// for (int i = 0; i < arp_table_len; i ++) {
	// 	uint8_t *ip_bytes = (uint8_t *) &arp_table[i].ip;
	//   	// fprintf(stderr, "%d %d %d %d -- ", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
		// ip_bytes = (uint8_t *) &arp_table[i].mac;
	  	// // fprintf(stderr, "%x %x %x %x %x %x\n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3], ip_bytes[4], ip_bytes[5]);
	// }


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
		fprintf(stderr, "Packet recieved from: ");
		// for (int i = 0; i < MAC_SIZE; i ++) {
			// fprintf(stderr, "%x ", eth_hdr->ether_shost[i]);
		// }
		// fprintf(stderr, "for: ");
		// for (int i = 0; i < MAC_SIZE; i ++) {
			// fprintf(stderr, "%x ", eth_hdr->ether_dhost[i]);
		// }
		// check lenght of packet??

		// check destination
		uint8_t *mac_address = malloc(MAC_SIZE * sizeof(u_int8_t));
		get_interface_mac(interface, mac_address);

		if (check_mac(mac_address, eth_hdr->ether_dhost) == 1) {
			// check type
			if (ntohs(eth_hdr->ether_type) == 0x0800) {
				fprintf(stderr, "IPv4\n");
				// initialize IP header
				struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header)); 
				
				// check if destintation
				uint8_t* recieved_ip = (uint8_t *)&ip_hdr->daddr;
				char *router_ip_char = get_interface_ip(interface);
				uint32_t router_ip = 0;
				int rc = inet_pton(AF_INET, router_ip_char, &router_ip);
				if (ip_hdr->daddr == router_ip) {
					// ICMP PROTOCOL
					// TODO
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
				if (ip_hdr->ttl < 1) {
					// fprintf(stderr, "TTL expired -- DROP\n");
					// TODO send ICMP
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
					continue;
				}

				// update checksum
				ip_hdr->check =
			        ~(~old_check + ~((uint16_t)old_ttl) + (uint16_t)ip_hdr->ttl) - 1;

				// get mac address from ip_dest
				struct arp_table_entry *next_hop = get_mac_address(best_route->next_hop);
				if (next_hop == NULL) {
					// fprintf(stderr, "something bad happen\n");
					// send ARP
				}
				// uint8_t *ip_bytes = (uint8_t *) &next_hop->ip;
				// // fprintf(stderr, "%d %d %d %d -- ", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
				// ip_bytes = (uint8_t *) next_hop->mac;
			  	// // fprintf(stderr, "%x %x %x %x %x %x\n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3], ip_bytes[4], ip_bytes[5]);

				// rewrite L2 address
				memcpy(eth_hdr->ether_dhost, next_hop->mac,
        	   		sizeof(eth_hdr->ether_dhost));
		    	
				// send packet
				send_to_link(best_route->interface, buf, len);
				}

			} else if (ntohs(eth_hdr->ether_type) == 0x0806) {
				// fprintf(stderr, "ARP\n");
			} else {
				// fprintf(stderr, "UNKNOWN -- DROP PACKET\n");
				continue;
			}

		} else {
			// fprintf(stderr, "WRONG MAC -> DROP PACKET\n");
			continue;
		}

	}
}

