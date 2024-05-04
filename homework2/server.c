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

void chat(int connfd) {
    char buffer[MAXREC];
    
    while (1) {
        bzero(buffer, MAXREC);
        read(connfd, buffer, MAXREC);
        printf("From client: %s\t To client : ", buffer); 
        bzero(buffer, MAXREC);
        fgets(buffer, MAXREC, stdin);
		write(connfd, buffer, MAXREC);
        if (strncmp("exit", buffer, 4) == 0) { 
            printf("Server Exit...\n"); 
            break; 
        } 
    }
}

int tcp_handler(int port) {
    int rc = 0;
    int tcp_sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // create socket
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd == -1) {
        fprintf(stderr, "socket tcp failed\n");
        return EXIT_FAILURE;
    }
    bzero(&servaddr, sizeof(servaddr));

    // fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // bind socket to ip
    rc = bind(tcp_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc != 0) {
        fprintf(stderr, "bind failed\n");
        return EXIT_FAILURE;
    }

    // listen
    rc = listen(tcp_sockfd, 5);
    if (rc == -1) {
        fprintf(stderr, "listen failed\n");
        return EXIT_FAILURE;
    }
    printf("server listen\n");

    len = sizeof(cli);

    // accept conenction
    connfd = accept(tcp_sockfd, (struct sock_addr *)&cli, &len);
    printf("accept\n");
    if (connfd < 0) {
        fprintf(stderr, "accept failed\n");
        return EXIT_FAILURE;
    }
    // chat with the client
    chat(connfd);

    // close socket
    close(tcp_sockfd);
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
    int rc = 0;
    // Cast the port
    int port = atoi(argv[1]);
    
    // rc = udp_handler(port);
    rc = tcp_handler(port);
    if (rc != 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
