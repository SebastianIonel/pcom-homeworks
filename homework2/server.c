#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/poll.h>

#ifndef TCP_NODELAY
#define TCP_NODELAY 0x01
#endif

#define MAXREC 2500
#define TYPE 50
#define INIT_PDFS 500

int main(int argc, char* argv[]) {
    // Check if port number is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char buffer[MAXREC];
    int rc = 0;
    // Cast the port
    int port = atoi(argv[1]);
    

    // Init UDP socket
    int udp_socketfd;
    struct sockaddr_in server_addr_udp, client_addr_udp;
    socklen_t len_udp = sizeof(client_addr_udp);
    
    // Create socket file descriptor
    udp_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socketfd == -1) { 
        fprintf(stderr, "socket creation failed"); 
        return EXIT_FAILURE;
    }

    // Fill server information 
    memset(&server_addr_udp, 0, sizeof(server_addr_udp)); 
    memset(&client_addr_udp, 0, len_udp); 
    server_addr_udp.sin_family = AF_INET; 
    server_addr_udp.sin_port = htons(port); 
    server_addr_udp.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind (udp_socketfd, (const struct sockaddr *)&server_addr_udp, sizeof(server_addr_udp)) == -1) {
        fprintf(stderr, "bind failed");
        return EXIT_FAILURE;
    }

    // Init TCP socket
    int tcp_sockfd, connfd, len_tcp;
    struct sockaddr_in server_addr_tcp, client_addr_tcp;

    // Create socket
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd == -1) {
        fprintf(stderr, "socket tcp failed\n");
        return EXIT_FAILURE;
    }
    bzero(&server_addr_tcp, sizeof(server_addr_tcp));

    // Disable Nagle`s algorithm
	int flag = 1;
	if (setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
		fprintf(stderr, "setsockopt failed\n");
		return EXIT_FAILURE;
	}  

    // Fill server information
    server_addr_tcp.sin_family = AF_INET;
    server_addr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr_tcp.sin_port = htons(port);

    // Bind socket to ip
    rc = bind(tcp_sockfd, (struct sockaddr *)&server_addr_tcp, sizeof(server_addr_tcp));
    if (rc != 0) {
        fprintf(stderr, "bind failed\n");
        return EXIT_FAILURE;
    }

    // Listen
    rc = listen(tcp_sockfd, 500);
    if (rc == -1) {
        fprintf(stderr, "listen failed\n");
        return EXIT_FAILURE;
    }    
    len_tcp = sizeof(client_addr_tcp);


    // Init the pool
    struct pollfd *pfds = malloc(INIT_PDFS * sizeof(struct pollfd));
    int nfds = 0;
    int used_nfds = 0;
    // pfds[0] == standard input
    pfds[0].fd = STDERR_FILENO;
    pfds[0].events = POLLIN;
    nfds ++;

    // pfds[1] == udp socket
    pfds[1].fd = udp_socketfd;
    pfds[1].events = POLLIN;
    nfds ++;

    // pfds[2] == tcp lisening socket
    pfds[2].fd = tcp_sockfd;
    pfds[2].events = POLLIN;
    nfds ++;
    used_nfds = nfds;

    
    while (1) {
        poll(pfds, nfds, -1);
        if ((pfds[0].revents & POLLIN) != 0) {
            // handle stdin
            char command[5];
            fgets(command, 4, stdin);
            if (strncmp("exit", command, 4) == 0) { 
                printf("Server Exit...\n"); 
                // TODO: close tcp clients
                break;
            } else {
                printf("Invalid command\n");
            }
        }
        else if((pfds[1].revents & POLLIN) != 0) {
            // handle recv message from udp
            // clear buffer
            printf("UDP\n");
            bzero(buffer, MAXREC);
            // get message from udp clients
            int rec = recvfrom(udp_socketfd, buffer, MAXREC, 0, (struct sockaddr *)&client_addr_udp, &len_udp);
            if (rec == -1) {
                perror("recvfrom failed");
                close(udp_socketfd); // Close socket and exit on error
                return EXIT_FAILURE;
            }
            // send message to tcp
            // TODO
            for (int i = 3; i < nfds; i ++) {
                write(pfds[i].fd, buffer, 56);
            }

        }
        else if((pfds[2].revents & POLLIN) != 0) {
            // handle new connection from tcp client
            // accept conenction
            printf("TCP\n");
            connfd = accept(tcp_sockfd, (struct sock_addr *)&client_addr_tcp, &len_tcp);
            printf("accept\n");
            if (connfd < 0) {
                fprintf(stderr, "accept failed\n");
                return EXIT_FAILURE;
            }
            pfds[nfds].fd = connfd;
            pfds[nfds].events = POLLIN;
            nfds ++;
            used_nfds ++;
            // TODO realloc
        } else {
            // handle message from tcp client
            for (int i = 3; i < nfds; i ++) {
                if ((pfds[i].revents & POLLIN) != 0) {
                    bzero(buffer, MAXREC);
                    read(pfds[i].fd, buffer, MAXREC);
                    printf("From client: %s: ", buffer);     
                    break;
                }
            }
            
        }
    }
    

    close(tcp_sockfd);
    close(udp_socketfd);
    return EXIT_SUCCESS;
}
