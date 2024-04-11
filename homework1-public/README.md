# Router - implementare IP, ARP & ICMP 

## Structuri de date folosite:
    -> struct packet: un buffer si lungimea sa
    
    -> struct route_table_entry: informatiile dintr-o intrare din
        tabela de rutare
    
    -> struct arp_table_entry: informatiile dintr-o intrare din
        tablea arp
    
    -> struct list: lista generica

    -> queue: retinehead sitail_ pentru o coada

    -> struct ether_header: header-ul de etherent

    -> struct iphdr: headerul ip

    -> struct arp_header: header-ul arp

    -> struct icmphdr: header-ul icmp

## Functiile din router.c

### 1. int main(...)
    -> se aloca memorie pentru tablele de rtable si arp
    -> rtable este sortat folosind qsort() si compare()
    -> in while(1) se primesc pachete pe orice interfata si se verifica daca 
        routerul este destinatia(check_mac(...)). Daca da atunci pachetele se
        "prelucreaza"
    -> se verifica tipul din ether_header:
        1. pt ip se apeleaza handle_ip_packet(...)
        2. pt arp se apeleaza arp_recieved(...)
        3. pt orice alt tip se da drop
    -> se elibereaza memoria

### 2. int check_mac(...)
    -> verifica daca my_address este egala cu recieved_address sau
        daca recieved address este egala cu adresa de broadcast

### 3. int compare(...)
    -> este functia folosita pentru qsort
    -> comapara 2 intrari din tabelul de routare in functie de prefix si masca

### 4. struct route_table_entry *find_best_route(...)
    -> returneaza intrarea din tabela de routare corespunzatoare pentru ip_dest
    -> pentru a fi mai eficient in locul unei cautari liniare se foloseste cautarea
        binara. Pentru a obtine cel mai lung prefix comun si in cazul in care se 
        gaseste un elemnt ce este egal cu "ip_dest & mask" se continua cautarea la
        dreapta pentru a obtine masa cea mai mare
    -> daca nu exista elementul se intoarece NULL

### 5. struct arp_table_entry* get_mac_address(...)
    -> cauta liniar prin tabela de arp si returneaza intrarea ce se potriveste cu
        ip-ul

### 6. void handle_ip_packet(...)
    -> se verifica daca ip_dest este ip ul routerului, daca da atunci inseamna ca 
        pachetul este de tip icmp si se apeleaza recv_icmp(...)
    -> se verifica checksum ul
    -> se verifica ttl si daca se poate ajunge la destinatie, daca nu, se apeleaza
        send_icmp() cu "type" si "code" corespunzatoare pt fiecare caz
    -> se actualizeaza checksum ul, se obtine adresa mac(direct din tabela sau daca
        nu exista se apeleaza send_arp(...))
    -> daca a existat adresa mac in tabela, se actualizeaza ether header cu adresele
        corespunzatoare pt sursa si destiantie
    -> se trimite pachetul mai departe

### 7. void arp_recieved(...)
    -> se verifica tipul pachetului
    -> pentru request(op == 1): se modifica op, sursele si destinatile din header si 
    se trimite inapoi un pachet de tip reply
    -> pentru reply(op == 2): se adauga sursa in tabelul arp si se scoate din coada 
    pachetele ce trebuie trimise catre acea destinati si se trimit

### 8. void send_arp(...)
    -> se creeaza un nou pachet de tip arp request
    -> la ether header se pune adresa de broadcast
    -> se seteaza campurile din arp header
    -> pachetul ce trebuia trimis intial se adauga in coada
    -> pachetul arp este trimis catre interfata de next hop

### 9. void recv_icmp(...)
    -> se verifica checksum ul
    -> pentru request se modifica pachetul intr-un reply pentru a fi trimis inapoi
    -> se actualizeaza checksum si pachetul este trimis, daca s a gasit mac ul 
    destinatiei in tabela, daca nu se apeleaza send_arp(...)

### 10. void send_icmp(...)
    -> se creeaza un nou buffer in care se umplu headerele de etherent ip si icmp si dupa 
        care se copiaza headerul ip si urmatorii 64 de biti din pcahetul "drop"
    -> achetul este trimis, daca s a gasit mac ul destinatiei in tabela, daca nu se
        apeleaza send_arp(...)