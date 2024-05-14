#include "client.h"
#include "helper.h"
#include "buffer.h"


int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        herror("error opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        herror("error connecting");

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
            herror("error reading response from socket");
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
            herror("error reading response from socket");
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
            herror("error writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}


char *post_request(int sockfd, char *url, char *content_type, char *body_data, char **cookies, int cookies_count) {
    char *message = calloc(5 * MAX_INPUT, sizeof(char));
    strcat(message, "POST ");
    char *content;
    char *content_length;
    // compute message
    compute_message(message, url);
    compute_message(message, "Host: " HOST);
    if (content_type != NULL) {
        content = calloc(MAX_INPUT, sizeof(char));
        strcat(content, "Content-Type: ");
        strcat(content, content_type);
        compute_message(message, content);
        content_length = calloc(MAX_INPUT, sizeof(char));
        sprintf(content_length, "Content-Length: %ld", strlen(body_data));
        compute_message(message, content_length);
    }
    if (cookies != NULL) {
        // TODO: add cookies
        char *the_cookies = calloc(cookies_count * BUFLEN, sizeof(char));
        strcat(the_cookies, "Cookie: ");
        for (int i = 0; i < cookies_count; i ++) {
            strcat(the_cookies, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(the_cookies, "; ");
            }
        }
        compute_message(message, the_cookies);
    }

    if (body_data != NULL) {
        compute_message(message, "");
        compute_message(message, body_data);
    }
    // printf("%s\n", message);
    // send message to server
    send_to_server(sockfd, message);

    if (content_type != NULL){
        free(content);
        free(content_length);
    }
    free(message);

    char *response;
    response = receive_from_server(sockfd);
    return response;
}

char *get_request(int sockfd, char *url, char **cookies, int cookies_cout, char *auth_token) {
    char *the_cookies;
    char *message = calloc(500, sizeof(char));
    strcat(message, "GET ");
    compute_message(message, url);
    compute_message(message, "Host: " HOST);
    if (cookies != NULL) {
        the_cookies = calloc(cookies_cout * BUFLEN, sizeof(char));
        strcat(the_cookies, "Cookie: ");
        for (int i = 0; i < cookies_cout; i ++) {
            strcat(the_cookies, cookies[i]);
            if (i != cookies_cout - 1) {
                strcat(the_cookies, "; ");
            }
        }
    }
    compute_message(message, the_cookies);
    
    if (auth_token  != NULL) {
        char *auth = calloc(BUFLEN, sizeof(char));
        strcat(auth, "Authorization: ");
        strcat(auth, auth_token);
        compute_message(message, auth);
    }

    compute_message(message, "");

    // printf("%s\n", message);
    
    send_to_server(sockfd, message);
    
    free(message);
    free(the_cookies);

    // printf("Received response\n");
    char *response;
    response = receive_from_server(sockfd);
    return response;
}

int register_user(int sockfd, char *username, char *password) {
    char *url = "/api/v1/tema/auth/register HTTP/1.1";
    char *content_type = "application/json";
    char *body_data = calloc(MAX_INPUT, sizeof(char));
    username[strlen(username) - 1] = '\0';
    password[strlen(password) - 1] = '\0';
    sprintf(body_data, "{\r\n   \"username\":\"%s\",\r\n    \"password\":\"%s\"\r\n}", username, password); 

    char *response = post_request(sockfd, url, content_type, body_data, NULL, 0);
    
    // free body_data
    free(body_data);
    
    // check response
    // printf("%s\n", response);
    if (strstr(response, "201 Created") != NULL) {
        return 1;
    } else {
        return -1;
    }
}


char *login_user(int sockfd, char *username, char *password) {
    char *url = "/api/v1/tema/auth/login HTTP/1.1";
    char *content_type = "application/json";
    char *body_data = calloc(MAX_INPUT, sizeof(char));
    username[strlen(username) - 1] = '\0';
    password[strlen(password) - 1] = '\0';
    sprintf(body_data, "{\r\n   \"username\":\"%s\",\r\n    \"password\":\"%s\"\r\n}", username, password); 


    char *response = post_request(sockfd, url, content_type, body_data, NULL, 0);

    char *check = strstr(response, "200 OK");
    char *cookie = calloc(BUFLEN, sizeof(char));
    if (check != NULL) {
        char *aux_cookie = strstr(response, "Set-Cookie: ") + strlen("Set-Cookie: ");
        int i = 0;
        while (aux_cookie[i] != ';') {
            i ++;
        }
        aux_cookie[i] = '\0';
        memcpy(cookie, aux_cookie, strlen(aux_cookie));
    } else {
        cookie = NULL;
    }
    
    free(body_data);
    return cookie;
}


char *enter_library(int sockfd, char **cookies, int cookies_count) {
    char *url = "/api/v1/tema/library/access HTTP/1.1";
    char *content_type = NULL;
    char *body_data = NULL;

    char *response = get_request(sockfd, url, cookies, cookies_count, NULL);
    printf("Response(%d) %s\n", strlen(response),response);
    
    if (strstr(response, "200 OK") != NULL) {
        // return token
        char *token = calloc(BUFLEN, sizeof(char));
        char *aux_token = strstr(response, "token") + strlen("token") + 3;
        int i = 0;
        while (aux_token[i] != '\"') {
            i ++;
        }
        aux_token[i] = '\0';
        memcpy(token, aux_token, strlen(aux_token));
        return token;
    } else {
        return NULL;
    }
}