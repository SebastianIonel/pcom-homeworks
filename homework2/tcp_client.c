#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <sys/poll.h>

#define MAXREC 2500
#ifndef TCP_NODELAY
#define TCP_NODELAY 0x01
#endif

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
			rc = read(sockfd, buffer, MAXREC);
			if (rc < 0) {
				fprintf(stderr, "write failed");
				return EXIT_FAILURE;
			}
			printf("Feedback: %s\n", buffer);
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