#include "client.h"
#include "helper.h"
#include "buffer.h"

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        herror("herror opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        herror("herror connecting");

    return sockfd;
}

void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            herror("herror reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            herror("herror reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            herror("herror writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}


char *post_request(int sockfd, char *url, char *content_type, char *body_data, char *cookies, int cookies_count) {
    char *message = calloc(500, sizeof(char));
    strcat(message, "POST ");
    // compute message
    compute_message(message, url);
    compute_message(message, "Host: " HOST);
    char *content = calloc(100, sizeof(char));
    strcat(content, "Content-Type: ");
    strcat(content, content_type);
    compute_message(message, content);
    char *content_length = malloc(100);
    sprintf(content_length, "Content-Length: %ld", strlen(body_data));
    compute_message(message, content_length);

    if (cookies != NULL) {
        // TODO: add cookies
    }

    compute_message(message, "");
    compute_message(message, body_data);

    printf("%s\n", message);
    // send message to server
    send_to_server(sockfd, message);

    free(content);
    free(content_length);
    free(message);

    char *response = calloc(10000, sizeof(char));
    response = receive_from_server(sockfd);
    return response;
}

int register_user(int sockfd, char *username, char *password) {
    char *url = "/api/v1/tema/auth/register HTTP/1.1";
    char *content_type = "application/json";
    char *body_data = malloc(100);
    username[strlen(username) - 1] = '\0';
    password[strlen(password) - 1] = '\0';
    sprintf(body_data, "{\r\n   \"username\":\"%s\",\r\n    \"password\":\"%s\"\r\n}", username, password); 
    char *cookies = NULL;
    int cookies_count = 0;

    char *response = post_request(sockfd, url, content_type, body_data, cookies, cookies_count);
    
    // free body_data
    free(body_data);
    
    // check response
    printf("%s\n", response);
    if (strstr(response, "201 Created") != NULL) {
        return 1;
    } else {
        return -1;
    }
}