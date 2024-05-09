#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <math.h>
#include <errno.h>

#define TYPE 50
#define INIT_PDFS 500
#define MAXREC 2500

#ifndef TCP_NODELAY
#define TCP_NODELAY 0x01
#endif

struct __attribute__((packed)) udp_message {
	char ip_address[16];
	char port[6];
	char topic[50];
	char type;
	char content[1500];
};

struct __attribute__((packed)) tcp_message {
	char start[2];
	struct udp_message info;
	char end[2];
};

struct __attribute__((packed)) tcp_commands {
	char start[2];
	// 0 - id
	// 1 - subscribe
	// 2 - unsubscribe
	// 3 - exit
	uint8_t command;
	char text[50];
	char end[2]; 
};

struct client_info {
	char *id;
	char **subscribes;
	int subscribes_number;
	char *ip_address;
	char port[6];
};