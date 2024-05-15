#include "client.h"
#include "helper.h"

int main() {
	int sockfd;
	char *cookies[256];
	int cookies_count = 0;
	char *auth_token = NULL;

	// connect to server
	sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);

	char command[COMMAND_LEN];

	while (1) {
		// wait command from user
		fgets(command, COMMAND_LEN, stdin);
		// check command
		if (strncmp(command, "exit", 4) == 0) {
			break;
		}
		else if (strncmp(command, "logout", 6) == 0) {
			// TODO: implement logout
			int ok = logout(sockfd, cookies, cookies_count, auth_token);
			if (ok < 0) {
				printf("LOGOUT FAILED. ERROR\n");
			} else {
				// free cookies
				for (int i = 0; i < cookies_count; i++) {
					free(cookies[i]);
				}
				cookies_count = 0;
				// free auth_token
				free(auth_token);
				auth_token = NULL;
				printf("LOGOUT SUCCESS\n");
			}
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
			char username[MAX_INPUT];
			char password[MAX_INPUT];
			printf("username=");
			fflush(stdout);
			fgets(username, MAX_INPUT, stdin);
			printf("password=");
			fflush(stdout);
			fgets(password, MAX_INPUT, stdin);
			cookies[cookies_count] = login_user(sockfd, username, password);
			if (cookies == NULL) {
				printf("LOGIN FAILED. ERROR\n");
			} else {
				cookies_count++;
				printf("LOGIN SUCCESS\n");
			}
		}
		else if (strncmp(command, "enter_library", 13) == 0) {
			// TODO: implement enter_library
			auth_token = enter_library(sockfd, cookies, cookies_count);
			if (auth_token == NULL) {
				printf("ENTER LIBRARY FAILED. ERROR\n");
			} else {
				printf("ENTER LIBRARY SUCCESS\n");
			}
		}
		else if (strncmp(command, "get_books", 9) == 0) {
			// TODO: implement get_books
			print_books(sockfd, cookies, cookies_count, auth_token);
		}
		else if (strncmp(command, "get_book", 8) == 0) {
			// TODO: implement get_book
			char id[10];
			printf("id=");
			fflush(stdout);
			fgets(id, 10, stdin);
			get_book(sockfd, cookies, cookies_count, auth_token, id);
		}
		else if (strncmp(command, "add_book", 8) == 0) {
			// TODO: implement add_book
			char title[MAX_INPUT];
			char author[MAX_INPUT];
			char genre[MAX_INPUT];
			char publisher[MAX_INPUT];
			char page_count[10];
			
			printf("title=");
			fflush(stdout);
			fgets(title, MAX_INPUT, stdin);
			printf("author=");
			fflush(stdout);
			fgets(author, MAX_INPUT, stdin);
			printf("genre=");
			fflush(stdout);
			fgets(genre, MAX_INPUT, stdin);
			printf("publisher=");
			fflush(stdout);
			fgets(publisher, MAX_INPUT, stdin);
			printf("page_count=");
			fflush(stdout);
			fgets(page_count, 10, stdin);
			
			// check corectness of page_count
			errno = 0;
			strtol(page_count, NULL, 10);
			if (errno != 0) {
				printf("FAILED: Invalid page_count\n");
				continue;
			}
			int ok = add_book(sockfd, cookies, cookies_count, auth_token, title, author, genre, publisher, page_count);
			if (ok < 0) {
				printf("ADD BOOK FAILED. ERROR\n");
			} else {
				printf("ADD BOOK SUCCESS\n");
			}
			
		}
		else if (strncmp(command, "delete_book", 11) == 0) {
			// TODO: implement delete_book
			char id[10];
			printf("id=");
			fflush(stdout);
			fgets(id, 10, stdin);
			int ok = delete_book(sockfd, cookies, cookies_count, auth_token, id);
			if (ok < 0) {
				printf("DELETE BOOK FAILED. ERROR\n");
			} else {
				printf("DELETE BOOK SUCCESS\n");
			}			
		} else {
			printf("Invalid command\n");
		}
		
	}


	// free cookies
	for (int i = 0; i < cookies_count; i++) {
		free(cookies[i]);
	}
	// close the connection
	close(sockfd);
}