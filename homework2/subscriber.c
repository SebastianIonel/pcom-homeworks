#include "protocol.h"

void print_info(struct tcp_message *msg) {
	// printf("%s -- TYPE: %u\n", msg->info.topic, msg->info.type);
	// <IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>
	// type = 0 == INT
	// type = 1 == SHORT_REAL
	// type = 2 == FLOAT
	// type = 3 == STRING
	if (msg->info.type == 0) {
		uint8_t sign;
		uint32_t value;
		memcpy(&sign, msg->info.content, 1);
		memcpy(&value, msg->info.content + 1, 4);
		value = ntohl(value);
		if (sign == 1) {
			value = -value;
		}
		printf("%s - INT - %d\n", msg->info.topic, value);
	}

	if (msg->info.type == 1) {
		uint16_t value;
		memcpy(&value, msg->info.content, 2);
		value = ntohs(value);
		double num = (double) value / 100;
		printf("%s - SHORT_REAL - %.2f\n", msg->info.topic, num);
	}

	if (msg->info.type == 2) {
		uint8_t power;
		uint8_t sign;
		uint32_t value;
		memcpy(&sign, msg->info.content, 1);
		memcpy(&value, msg->info.content + 1, 4);
		memcpy(&power, msg->info.content + 5, 1);
		value = ntohl(value);
		double num = (double) value / pow(10, power);
		if (sign == 1)
			num = -num;
		printf("%s - FLOAT - %.*f\n", msg->info.topic, power, num);
	}

	if (msg->info.type == 3) {
		printf("%s - STRING - %s\n", msg->info.topic, msg->info.content);
	}

	// printf("%s - %s - ", msg->info.topic, msg->info.type);

	// // print content based on type
	// if (msg->info.type == 0) {
	// 	uint8_t sign;
	// 	uint32_t value;
	// 	memcpy(&sign, msg->info.content, 1);
	// 	memcpy(&value, msg->info.content + 1, 4);
	// 	value = ntohl(value);
	// 	if (sign == 1) {
	// 		value = -value;
	// 	}
	// 	printf("%d\n", value);
	// }
	// if (msg->info.type == 1) {
	// 	uint16_t value;
	// 	memcpy(&value, msg->info.content, 2);
	// 	value = ntohs(value);
	// 	double num = (double) value / 100;
	// 	printf("%.2f\n", num);
	// }

	// if (msg->info.type == 2) {
	// 	uint8_t power;
	// 	uint8_t sign;
	// 	uint32_t value;
	// 	memcpy(&sign, msg->info.content, 1);
	// 	memcpy(&value, msg->info.content + 1, 4);
	// 	memcpy(&power, msg->info.content + 5, 1);
	// 	value = ntohl(value);
	// 	double num = (double) value / pow(10, power);
	// 	if (sign == 1)
	// 		num = -num;
	// 	printf("%.*f\n", power, num);
	// }

	// if (msg->info.type == 3) {
	// 	printf("%s\n", msg->info.content);
	// }
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
			// if subscribe command
			if (strncmp(buffer, "subscribe", 9) == 0) {
				struct tcp_commands *cmd = malloc(sizeof(struct tcp_commands));
				bzero(cmd, sizeof(struct tcp_commands));
				memcpy(cmd->start, "##", 2);
				cmd->command = 1;
				memcpy(cmd->text, buffer + 10, strlen(buffer) - 10);
				memcpy(cmd->end, "##", 2);
				rc = write(sockfd, cmd, sizeof(struct tcp_commands));
				if (rc < 0) {
					fprintf(stderr, "write failed");
					return EXIT_FAILURE;
				}
				// give feedback
				printf("Subscribed to topic %s", buffer + 10);
				free(cmd);
			}
			else if (strncmp(buffer, "unsubscribe", 11) == 0) {
				struct tcp_commands *cmd = malloc(sizeof(struct tcp_commands));
				bzero(cmd, sizeof(struct tcp_commands));
				memcpy(cmd->start, "##", 2);
				cmd->command = 2;
				memcpy(cmd->text, buffer + 12, strlen(buffer) - 12);
				memcpy(cmd->end, "##", 2);
				rc = write(sockfd, cmd, sizeof(struct tcp_commands));
				if (rc < 0) {
					fprintf(stderr, "write failed");
					return EXIT_FAILURE;
				}
				// give feedback
				printf("Unsubscribed from topic %s", buffer + 12);
				free(cmd);
			}
			else if (strncmp(buffer, "exit", 4) == 0) {
				struct tcp_commands *cmd = malloc(sizeof(struct tcp_commands));
				bzero(cmd, sizeof(struct tcp_commands));
				memcpy(cmd->start, "##", 2);
				cmd->command = 3;
				memcpy(cmd->text, "exit", 4);
				memcpy(cmd->end, "##", 2);
				rc = write(sockfd, cmd, sizeof(struct tcp_commands));
				if (rc < 0) {
					fprintf(stderr, "write failed");
					return EXIT_FAILURE;
				}
				free(cmd);
			}
			else {
				fprintf(stderr, "Invalid command");
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
        fprintf(stderr, "Usage: %s <id_client> <ip_server> <port>", argv[0]);
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

	// send id to server
	struct tcp_commands *cmd = malloc(sizeof(struct tcp_commands));
	bzero(cmd, sizeof(struct tcp_commands));
	memcpy(cmd->start, "##", 2);
	cmd->command = 0;
	memcpy(cmd->text, id, strlen(id));
	memcpy(cmd->end, "##", 2);
	rc = write(sockfd, cmd, sizeof(struct tcp_commands));
	if (rc < 0) {
		fprintf(stderr, "write failed\n");
		return EXIT_FAILURE;
	}
	
	
	// chat
	rc = chat(sockfd);
	if (rc != 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}