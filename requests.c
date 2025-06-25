#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

// Cerere GET
char *compute_get_request(char *host, char *url, char *query_params, 
                            char **headers, int headers_count,
                            char **cookies, int cookies_count)
{
    // Alocam memorie pt mesajul HTTP
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Adaugam header pt Host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Adaugam alte headere (daca e necesar)
    // De exemplu acces token
    if (headers != NULL) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    // Adaugam cookies
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    // Finalizam mesajul
    compute_message(message, "");

    free(line);
    return message;
}

// Cerere POST - pentru a crea ceva complet nou pe server
char *compute_post_request(char *host, char *url, char *content_type, 
                           char **body_data, int body_data_fields_count, 
                           char **headers, int headers_count, 
                           char **cookies, int cookies_count)
{
    // Fata de GET, mai avem un buffer pentru cerere
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = NULL;

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (headers != NULL) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Construim cererea
    if (body_data_fields_count == 1) {
        body_data_buffer = strdup(body_data[0]);
    } else {
        body_data_buffer = calloc(LINELEN, sizeof(char));
        for (int i = 0; i < body_data_fields_count; i++) {
            strcat(body_data_buffer, body_data[i]);
            if (i < body_data_fields_count - 1) {
                strcat(body_data_buffer, "&");
            }
        }
    }

    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}

// DELETE
char *compute_delete_request(char *host, char *url, char **headers, int headers_count,
                            char **cookies, int cookies_count) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (headers != NULL) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    free(line);
    return message;
}

// PUT - pentru a actualiza ceva deja existent pe server
char *compute_put_request(char *host, char *url, char *content_type, 
                           char **body_data, int body_data_fields_count, 
                           char **headers, int headers_count, 
                           char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = NULL;

    sprintf(line, "PUT %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (headers != NULL) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    if (body_data_fields_count == 1) {
        body_data_buffer = strdup(body_data[0]);
    } else {
        body_data_buffer = calloc(LINELEN, sizeof(char));
        for (int i = 0; i < body_data_fields_count; i++) {
            strcat(body_data_buffer, body_data[i]);
            if (i < body_data_fields_count - 1) {
                strcat(body_data_buffer, "&");
            }
        }
    }

    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}
