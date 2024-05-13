#include "client.h"
#include "helper.h"

int main() {
	int sockfd;
	char *message;
	char *response;
	
	// connect to server
	sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);

	char command[COMMAND_LEN];
	// wait command from user
	while (1) {
		fgets(command, COMMAND_LEN, stdin);
		if (strncmp(command, "exit", 4) == 0) {
			break;
		}
		else if (strncmp(command, "logout", 6) == 0) {
			// TODO: implement logout
		}
		else if (strncmp(command, "register", 8) == 0) {
			// TODO: implement register
			printf("username=");
			fflush(stdout);
			char username[MAX_INPUT];
			char password[MAX_INPUT];
			fgets(username, MAX_INPUT, stdin);
			printf("password=");
			fflush(stdout);
			fgets(password, MAX_INPUT, stdin);
			int ok = register_user(sockfd, username, password);
			if (ok < 0) {
				printf("REGISTER FAILED. ERROR\n");
			} else {
				printf("REGISTER SUCCESS\n");
			}
		}
		else if (strncmp(command, "login", 5) == 0) {
			// TODO: implement login
		}
		else if (strncmp(command, "enter_library", 13) == 0) {
			// TODO: implement enter_library
		}
		else if (strncmp(command, "get_books", 9) == 0) {
			// TODO: implement get_books
		}
		else if (strncmp(command, "get_book", 8) == 0) {
			// TODO: implement get_book
		}
		else if (strncmp(command, "add_book", 8) == 0) {
			// TODO: implement add_book
		}
		else if (strncmp(command, "delete_book", 11) == 0) {
			// TODO: implement delete_book
		} else {
			printf("Invalid command\n");
		}
		
	}

}