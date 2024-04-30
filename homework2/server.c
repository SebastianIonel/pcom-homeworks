#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define MAXREC 2500
#define TYPE 50

int tcp_reciver(int port) {
    return 0;
}

int tcp_sender(int port) {
    return 0;   
}

int udp_handler(int port) {
    // Init buffer
    char buffer[MAXREC];
    
    // Init UDP socket
    int udp_socketfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);
    
    // Create socket file descriptor
    if ((udp_socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { 
        perror("socket creation failed"); 
        return EXIT_FAILURE;
    }

    // Fill server information 
    memset(&server_addr, 0, sizeof(server_addr)); 
    memset(&client_addr, 0, sizeof(client_addr)); 
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(port); 
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind (udp_socketfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        return EXIT_FAILURE;
    }

    while (1) {
        int rec = recvfrom(udp_socketfd, buffer, MAXREC, 0, (struct sockaddr *)&client_addr, &len);
        if (rec == -1) {
            perror("recvfrom failed");
            close(udp_socketfd); // Close socket and exit on error
            return EXIT_FAILURE;
        }
        char topic[TYPE + 1];
        // buffer[rec] = '\0';
        // printf("Client(size: %d): %s\n", rec, buffer);
        int i = 0;
        for (i = 0; i < TYPE; i ++) {
            topic[i] = buffer[i];
        }
        topic[TYPE] = '\0';
        uint8_t type = (uint8_t) buffer[TYPE];
        
        
        printf("\tClient -- size: %d\n", rec);
        printf("Topic: %s\n", topic);
        printf("Type: %u\n", type);

    }
    

    close(udp_socketfd);
}

int main(int argc, char* argv[]) {
    // Check if port number is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Cast the port
    int port = atoi(argv[1]);
    
    udp_handler(port);
    
    return EXIT_SUCCESS;
}
