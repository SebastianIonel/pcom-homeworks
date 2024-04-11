# Router - implementare IP, ARP & ICMP 

## Structuri de date folosite:
    -> *struct packet*: un buffer si lungimea sa
    
    -> *struct route_table_entry*: informatiile dintr-o intrare din
        tabela de rutare
    
    -> *struct arp_table_entry*: informatiile dintr-o intrare din
        tablea arp
    
    -> *struct list*: lista generica

    -> *queue*: retine*head* si*tail_ pentru o coada

    -> *struct ether_header*: header-ul de etherent

    -> *struct iphdr*: headerul ip

    -> *struct arp_header*: header-ul arp

    -> *struct icmphdr*: header-ul icmp

## Functiile din router.c

    1.**int main(...)**
    -> se aloca memorie pentru tablele de rtable si arp
    -> rtable este sortat folosind *qsort()* si *compare()*
    -> in *while(1)* se primesc pachete pe orice interfata si se verifica daca 
        routerul este destinatia(*check_mac(...)*). Daca da atunci pachetele se "prelucreaza"
    -> se verifica tipul din *ether_header*:
        1. pt ip se apeleaza *handle_ip_packet(...)*
        2. pt arp se apeleaza*arp