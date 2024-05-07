#include "protocol.h"

void print_info(struct tcp_message *msg) {
	printf("%s -- TYPE: %u\n", msg->info.topic, msg->info.type);
}

int chat(int sockfd) {
	char buffer[MAXREC];
	int rc = 0;
	
	// clear buffer
	bzero(buffer, MAXREC);

	struct pollfd pfds[2];
	int nfds = 0;

	// add stdin
	pfds[nfds].fd = STDIN_FILENO;
	pfds[nfds].events = POLLIN;
	nfds ++;

	// add socket
	pfds[nfds].fd = sockfd;
	pfds[nfds].events = POLLIN;
	nfds ++;

	while (1) {
		poll(pfds, nfds, -1);
		if ((pfds[STDIN_FILENO].revents & POLLIN) != 0) {
			// read from stdin
			fgets(buffer, MAXREC, stdin);
			// send to server
			rc = write(sockfd, buffer, MAXREC);
			if (rc < 0) {
				fprintf(stderr, "write failed");
				return EXIT_FAILURE;
			}
			
			// clear buffer
			bzero(buffer, MAXREC);
		} 
		else if((pfds[1].revents & POLLIN) != 0) {
			// recieved from server
			// should be in a while and assemble
			struct tcp_message *tcp_msg = NULL;
			char message[MAXREC], aux[MAXREC], *msg_point = message;
			int end = 0;
			int start = 0;
			int len = 0;
			while (1) {
				rc = read(sockfd, buffer, MAXREC);
				if (rc < 0) {
					fprintf(stderr, "write failed");
					return EXIT_FAILURE;
				}
				len += rc;
				memcpy(msg_point, buffer, rc);
				msg_point = msg_point + rc;				
				// printf("LENGTH: %d\n", len);
				
				while (1) {
					start = 0;
					end = 0;
					for (int i = 0; i < len; i ++) {
					if (message[i] == 35 && message[i + 1] == 35 && start == 0) {
						start = 1;
					}
					if (message[i] == 35 && message[i + 1] == 35 && start == 1) {
						end = 1;
					}
					// printf("%d - ", buffer[i]);
				}
					if (start == 1 && end == 1) {
						tcp_msg = (struct tcp_message *)message;
						
						print_info(tcp_msg);
						// clear message
						len = len - sizeof(struct tcp_message);
						memcpy(aux, message + sizeof(struct tcp_message), len);
						bzero(message, MAXREC);
						memcpy(message, aux, len);
						msg_point = message + len;
						
						start = 0;
						end = 0;
					} else {
						break;
					}
				}
				
			}
			// printf("Feedback: %s\n", buffer);
			if ((strncmp(buffer, "exit", 4)) == 0) {
				printf("Client Exit...\n");
				break;
			}
			// clear buffer
			bzero(buffer, MAXREC);
		}
	}
}
 
// argv[1] == ID client
// argv[2] == IP server
// argv[3] == PORT
int main(int argc, char *argv[]) {
	if (argc != 4) {
        fprintf(stderr, "Usage: %s <id_client> <ip_server> <port>\n", argv[0]);
        return EXIT_FAILURE;
	}
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int rc = 0;
	char *id = argv[1];
	char *ip = argv[2];
	int port = atoi(argv[3]);
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;

	// create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		fprintf(stderr, "socket failed\n");
		return EXIT_FAILURE;
	}
	// set memory to 0
	bzero(&servaddr, sizeof(servaddr));

	// disable Nagle`s algorithm
	int flag = 1;
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
		fprintf(stderr, "setsockopt failed\n");
		return EXIT_FAILURE;
	}
	
	// fill server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port = htons(port);

	// connect client to server
	rc = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (rc != 0) {
		fprintf(stderr, "connect failed\n");
		return EXIT_FAILURE;
	}

	// chat
	rc = chat(sockfd);
	if (rc != 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}