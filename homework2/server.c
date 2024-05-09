#include "protocol.h"

int main(int argc, char* argv[]) {
    // Check if port number is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    struct client_info *clients = malloc(INIT_PDFS * sizeof(struct client_info));
    int clients_number = 0;
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
                // printf("Server Exit...\n"); 
                // TODO: close tcp clients
                for (int i = 3; i < nfds; i ++) {
                    close(pfds[i].fd);
                }
                close(tcp_sockfd);
                close(udp_socketfd);
                return EXIT_SUCCESS;
            } else {
                printf("Invalid command\n");
            }
        }
        else if((pfds[1].revents & POLLIN) != 0) {
            // handle recv message from udp
            // clear buffer
            // printf("UDP\n");
            bzero(buffer, MAXREC);
            // get message from udp clients
            int rec = recvfrom(udp_socketfd, buffer, MAXREC, 0, (struct sockaddr *)&client_addr_udp, &len_udp);
            if (rec == -1) {
                perror("recvfrom failed");
                close(udp_socketfd); // Close socket and exit on error
                return EXIT_FAILURE;
            }
            struct tcp_message tcp_msg;
            tcp_msg.start[0] = 35;
            tcp_msg.start[1] = 35;
            memcpy(tcp_msg.info.topic, buffer, 50);
            tcp_msg.info.type = buffer[TYPE];
            memcpy(tcp_msg.info.content, buffer + TYPE + 1, 1500);
            memcpy(tcp_msg.info.ip_address, inet_ntoa(client_addr_udp.sin_addr), 16);
            uint16_t aux_port = ntohs(client_addr_udp.sin_port);
            sprintf(tcp_msg.info.port, "%hu", port);
            tcp_msg.end[0] = 35;
            tcp_msg.end[1] = 35;
            
            // send message to tcp
            char *p = (char *)&tcp_msg;
            // printf("Message received from %s:%hu -- %d\n", tcp_msg.info.ip_address, aux_port, nfds);
            for (int i = 3; i < nfds; i ++) {
                // redirect message only to clients that are subscribed to the topic
                // printf("Client %d, id: %s\n", i - 3, clients[i - 3].id);
                // printf("Search topic: %s in %d \n", tcp_msg.info.topic, clients[i - 3].subscribes_number);
                for (int j = 0; j < clients[i - 3].subscribes_number; j ++) {
                    // printf("Subscribed to: %s\n", clients[i - 3].subscribes[j]);
                    if (strncmp(clients[i - 3].subscribes[j], tcp_msg.info.topic, strlen(tcp_msg.info.topic)) == 0) {
                        // printf("Send to %d\n", i);
                        int send = 0, remaining = sizeof(tcp_msg);
                        int current_send = 0;
                        while (remaining > 0) {
                            current_send = write(pfds[i].fd, (p + send), remaining);
                            remaining -= current_send;
                            send += current_send;
                        }
                    }
                }
            }

        }
        else if((pfds[2].revents & POLLIN) != 0) {
            // handle new connection from tcp client
            // accept conenction
            connfd = accept(tcp_sockfd, (struct sockaddr *)&client_addr_tcp, (socklen_t *)&len_tcp);
            
            if (connfd < 0) {
                fprintf(stderr, "accept failed\n");
                return EXIT_FAILURE;
            }
            pfds[nfds].fd = connfd;
            pfds[nfds].events = POLLIN;
            nfds ++;
            used_nfds ++;
            // check if there is space for new client
            if (clients_number == INIT_PDFS) {
                clients = realloc(clients, (clients_number + INIT_PDFS) * sizeof(struct client_info));
            }

            // create new client info
            uint16_t aux_port = ntohs(client_addr_tcp.sin_port);
            sprintf(clients[clients_number].port, "%hu", aux_port);
            // printf("New client(%d) connected from %s:%s\n", clients_number, inet_ntoa(client_addr_tcp.sin_addr), clients[clients_number].port);
            clients[clients_number].ip_address = inet_ntoa(client_addr_tcp.sin_addr);
            clients[clients_number].subscribes = malloc(INIT_PDFS * sizeof(char *));
            clients[clients_number].subscribes_number = 0;
            // printf("Client %d, ip: %s, port: %s\n", clients_number, clients[clients_number].ip_address, clients[clients_number].port);

        } else {
            // handle message from tcp client
            for (int i = 3; i < nfds; i ++) {
                if ((pfds[i].revents & POLLIN) != 0) {
                    // bzero(buffer, MAXREC);
                    char read_buffer[sizeof(struct tcp_commands)];
                    rc = read(pfds[i].fd, read_buffer, sizeof(struct tcp_commands));
                    if (rc < 0) {
                        fprintf(stderr, "read failed");
                        return EXIT_FAILURE;
                    }

                    struct tcp_commands *tcp_cmd = (struct tcp_commands *)read_buffer;
                    if (tcp_cmd->command == 0) {
                        // id
                        // check if id exists
                        for (int j = 0; j < clients_number; j ++) {
                            if (strncmp(clients[j].id, tcp_cmd->text, 50) == 0) {
                                fprintf(stderr, "Client %s already connected\n", tcp_cmd->text);
                                // close client
                                close(pfds[i].fd);
                                // remove from pool
                                for (int k = i; k < nfds - 1; k ++) {
                                    pfds[k] = pfds[k + 1];
                                }
                                nfds --;
                                used_nfds --;

                                break;
                            }
                        }
                        clients[clients_number].id = malloc(50);
                        memcpy(clients[clients_number].id, tcp_cmd->text, 50);
                        clients_number ++;
                        // print command
                        printf("New client %s connected from %s:%s\n", tcp_cmd->text, clients[clients_number - 1].ip_address, clients[clients_number - 1].port);
                    } else if (tcp_cmd->command == 1) {
                        // subscribe
                        // check if topic is already subscribed
                        int subscribed = 0;

                        for (int j = 0; j < clients[i - 3].subscribes_number; j ++) {
                            if (strncmp(clients[i - 3].subscribes[j], tcp_cmd->text, 50) == 0) {
                                subscribed = 1;
                                break;
                            }
                        }
                        if (subscribed == 0) {
                            clients[i - 3].subscribes[clients[i - 3].subscribes_number] = malloc(50);
                            memcpy(clients[i - 3].subscribes[clients[i - 3].subscribes_number], tcp_cmd->text, 50);
                            clients[i - 3].subscribes_number ++;
                        }

                    } else if (tcp_cmd->command == 2) {
                        // unsubscribe
                        for (int j = 0; j < clients[i - 3].subscribes_number; j ++) {
                            if (strncmp(clients[i - 3].subscribes[j], tcp_cmd->text, 50) == 0) {
                                free(clients[i - 3].subscribes[j]);
                                for (int k = j; k < clients[i - 3].subscribes_number - 1; k ++) {
                                    clients[i - 3].subscribes[k] = clients[i - 3].subscribes[k + 1];
                                }
                                clients[i - 3].subscribes_number --;
                            }
                        }
                    } else if (tcp_cmd->command == 3) {
                        // exit
                        for (int j = 0; j < clients_number; j ++) {
                            if (strncmp(clients[j].id, tcp_cmd->text, 50) == 0) {
                                printf("Client %s disconnected.\n", tcp_cmd->text);
                                for (int k = 0; k < clients[j].subscribes_number; k ++) {
                                    free(clients[j].subscribes[k]);
                                }
                                free(clients[j].subscribes);
                                free(clients[j].id);
                                for (int k = j; k < clients_number - 1; k ++) {
                                    clients[k] = clients[k + 1];
                                }
                                clients_number --;
                            }
                        }
                    }
                    // print tcp_cmd
                    // printf("Command: %d\n", tcp_cmd->command);
                    // printf("Text: %s\n", tcp_cmd->text);

                    
                }
            }
        }
    }
    

    close(tcp_sockfd);
    close(udp_socketfd);
    return EXIT_SUCCESS;
}
