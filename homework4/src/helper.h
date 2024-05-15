int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

int register_user(int sockfd, char *username, char *password);

void send_to_server(int sockfd, char *message);

void compute_message(char *message, const char *line);

char *post_request(int sockfd, char *url, char *content_type, char *body_data, 
	char **cookies, int cookies_count, char *auth_token);

char *login_user(int sockfd, char *username, char *password);

char *enter_library(int sockfd, char **cookies, int cookies_count);

void print_books(int sockfd, char **cookies, int cookies_count, char *auth_token);

int add_book(int sockfd, char **cookies, int cookies_count, char *auth_token, char *title,
	char *author, char *genre, char *publisher, char *page_count);

void get_book(int sockfd, char **cookies, int cookies_count, char *auth_token, char *id);

char *delete_request(int sockfd, char *url, char *content_type, char **cookies, int cookies_count, char *auth_token);

int delete_book(int sockfd, char **cookies, int cookies_count, char *auth_token, char *id);

int logout(int sockfd, char **cookies, int cookies_count, char *auth_token);