#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"
#define MAX_MOVIES 100
#define SERVER_IP "63.32.125.183"
#define SERVER_PORT 8081

char cookie_admin[1024] = "";
char cookie_user[1024] = "";
char jwt_token[1024] = "";
int movie_ids[MAX_MOVIES];
int movie_count = 0;

void clear_stdin_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void login_admin() {
    setvbuf(stdout, NULL, _IONBF, 0); 
     if (strlen(cookie_admin) > 0) {
        printf("ERROR: Sunteti deja autentificat ca admin.\n");
        return;
    }
    char username[50], password[50];
    char *message, *response;
    int sockfd = -1;

    printf("username=");
    clear_stdin_buffer();
    if (fgets(username, sizeof(username), stdin) == NULL) {
        perror("fgets username");
        return;
    }
    username[strcspn(username, "\n")] = '\0';

    printf("password=");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        perror("fgets password");
        return;
    }
    password[strcspn(password, "\n")] = '\0';

    // Deschidem conexiunea
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    // Creez payload ("continutul" mesajului)
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);

    char *payload = json_serialize_to_string(root_value);
    
    char *cookies[] = {cookie_user};
    // cerere POST cu payload si cookie de admin
    message = compute_post_request(SERVER_IP, "/api/v1/tema/admin/login", "application/json", 
                                    &payload, 1, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

     if (response == NULL) {
        printf("ERROR: Nu s-a primit niciun raspuns de la server.\n");
        close_connection(sockfd);
        json_free_serialized_string(payload);
        json_value_free(root_value);
        free(message);
        return;
    }

    if (strstr(response, "200 OK")) {
        char *cookie_start = strstr(response, "Set-Cookie: ");
        if (cookie_start) {
            sscanf(cookie_start, "Set-Cookie: %1023[^\r\n]", cookie_admin);
            printf("SUCCESS: Admin autentificat cu succes\n");
        } else {
            printf("ERROR: nu avem cookie\n");
        }
    } else {
        printf("ERROR: date gresite\n");
    }

    json_free_serialized_string(payload);
    json_value_free(root_value);
    close_connection(sockfd);
    free(message);
    free(response);
}

void add_user() {

    char username[50], password[50];
    char *message, *response;
    int sockfd = -1;


    printf("username=");
    clear_stdin_buffer();
    if (fgets(username, sizeof(username), stdin) == NULL) {
        perror("fgets username");
        return;
    }
    username[strcspn(username, "\n")] = '\0';

    printf("password=");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        perror("fgets password");
        return;
    }
    password[strcspn(password, "\n")] = '\0';

    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);

    char *payload = json_serialize_to_string(root_value);

    char *cookies[] = {cookie_admin};
    message = compute_post_request(SERVER_IP, "/api/v1/tema/admin/users", "application/json",
                                     &payload, 1, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK") || strstr(response, "201 CREATED")) {
        printf("SUCCESS: Utilizator adaugat cu succes!\n");
    } else if (strstr(response, "409 CONFLICT")) {
        printf("ERROR: Utilizatorul deja exista.\n");
    } else {
        printf("ERROR: Eroare la adaugarea utilizatorului.\n");
        printf("%s\n", response);
    }

    json_free_serialized_string(payload);
    json_value_free(root_value);
    close_connection(sockfd);
    free(message);
    free(response);
}

void get_users() {

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char *cookies[] = {cookie_admin};
    message = compute_get_request(SERVER_IP, "/api/v1/tema/admin/users", 
                                    NULL, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

        JSON_Value *root_value = json_parse_string(strstr(response, "{"));

        // Luam userii din mesajul serverului
        JSON_Object *root_object = json_value_get_object(root_value);
        JSON_Array *users_array = json_object_get_array(root_object, "users");

        //Parsez fiecare user si afisez campurile dorite
        for (size_t i = 0; i < json_array_get_count(users_array); i++) {
            JSON_Object *user = json_array_get_object(users_array, i);
            double id_double = json_object_get_number(user, "id");
            const char *username = json_object_get_string(user, "username");
            const char *password = json_object_get_string(user, "password");
            printf("#%.0f %s:%s\n", id_double, username ? username : "N/A", password ? password : "N/A");
            
        }

        json_value_free(root_value);

    close_connection(sockfd);
    free(message);
    free(response);
}

void delete_user() {

    char username[50];
    char *message, *response;
    int sockfd = -1;

    printf("username=");
    clear_stdin_buffer();
    if (fgets(username, sizeof(username), stdin) == NULL) {
        perror("fgets username");
        return;
    }
    username[strcspn(username, "\n")] = '\0';

    if (strlen(username) == 0) {
        printf("ERROR: Username nu poate fi gol.\n");
        return;
    }

    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/admin/users/%s", username);

    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    
    char *cookies[] = {cookie_admin};
    message = compute_delete_request(SERVER_IP, url, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Utilizatorul %s a fost sters cu succes.\n", username);
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Utilizatorul %s nu a fost gasit.\n", username);
    } else {
        printf("ERROR:");
    }

    close_connection(sockfd);
    free(message);
    free(response);
}


void logout_admin() {

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char *cookies[] = {cookie_admin};
    message = compute_get_request(SERVER_IP, "/api/v1/tema/admin/logout",
                                    NULL, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Delogare cu succes!\n");
        // Golim cookie-ul de admin
        cookie_admin[0] = '\0';
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void login() {

    char admin_username[50], username[50], password[50];
    char *message, *response;
    int sockfd = -1;

    printf("admin_username=");
    clear_stdin_buffer();
    if (fgets(admin_username, sizeof(admin_username), stdin) == NULL) return;
    admin_username[strcspn(admin_username, "\n")] = '\0';

    printf("username=");
    if (fgets(username, sizeof(username), stdin) == NULL) return;
    username[strcspn(username, "\n")] = '\0';

    printf("password=");
    if (fgets(password, sizeof(password), stdin) == NULL) return;
    password[strcspn(password, "\n")] = '\0';

    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "admin_username", admin_username);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    // Creez payload cu datele userului la care vreau sa ma autentific
    char *payload = json_serialize_to_string(root_value);

    char *cookies[] = {cookie_admin};
    message = compute_post_request(SERVER_IP, "/api/v1/tema/user/login", "application/json", 
                                    &payload, 1, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    cookie_user[0] = '\0';

    if (response == NULL) {
        printf("ERROR: Nu s-a primit niciun raspuns de la server.\n");
        close_connection(sockfd);
        json_free_serialized_string(payload);
        json_value_free(root_value);
        free(message);
        return;
    }

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Utilizator %s autentificat cu succes sub adminul %s\n", username, admin_username);
        // Cautam headerul pentru cookie in raspuns
        char *cookie_start = strstr(response, "Set-Cookie: ");
        if (cookie_start) {
            cookie_start += strlen("Set-Cookie: ");
            char *cookie_end = strstr(cookie_start, ";");
            if (cookie_end) {
                size_t cookie_len = cookie_end - cookie_start;
                if (cookie_len < sizeof(cookie_user)) {
                    // Copiem cookie de user in variabila globala
                    strncpy(cookie_user, cookie_start, cookie_len);
                    cookie_user[cookie_len] = '\0';
                } else {
                    printf("ERROR: Cookie-ul primit este prea mare.\n");
                }
            } else {
                printf("ERROR: Cookie-ul nu a fost gasit în raspuns.\n");
            }
        } else {
            printf("WARNING: Autentificare reusita, dar nu am primit un cookie de sesiune.\n");
        }
    }  else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Sunteti deja autentificat ca admin. %s.\n", username);
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Utilizatorul %s nu exista sub adminul %s. Contactati adminul pentru a fi creat.\n", username, admin_username);
    } else {
        printf("ERROR: Eroare la autentificare. Raspuns server:\n%s\n", response);
    }

    // Cleanup
    json_free_serialized_string(payload);
    json_value_free(root_value);
    close_connection(sockfd);
    free(message);
    free(response);
}

void get_access() {

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char *cookies[] = {cookie_user};
    message = compute_get_request(SERVER_IP, "/api/v1/tema/library/access", NULL, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Acces acordat la librarie!\n");
         JSON_Value *root_value = json_parse_string(strstr(response, "{"));
        if (root_value == NULL) {
            printf("ERROR: Raspuns invalid de la server.\n");
            free(response);
            return;
        }
        // Extragem token de acces din raspuns
        JSON_Object *root_object = json_value_get_object(root_value);
        const char *token = json_object_get_string(root_object, "token");
        if (token) {
            strncpy(jwt_token, token, sizeof(jwt_token) - 1);
            jwt_token[sizeof(jwt_token) - 1] = '\0';
        } else {
            printf("ERROR: Nu s-a putut extrage token-ul JWT.\n");
        }

        json_value_free(root_value);
    } else if (strstr(response, "401 Unauthorized")) {
        printf("ERROR: Nu sunteti autentificat. Folositi login_admin sau login.\n");
    } else {
        printf("ERROR: Eroare la obtinerea accesului. Raspuns server:\n%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void logout() {

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char *cookies[] = {cookie_user};
    message = compute_get_request(SERVER_IP, "/api/v1/tema/user/logout", NULL, NULL, 0, cookies, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Delogare utilizator cu succes!\n");
        // Golim token de acces si cookie de user
        cookie_user[0] = '\0';
        jwt_token[0] = '\0';
    } else {
        printf("ERROR: Eroare la delogarea utilizatorului.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void get_movies() {

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    // Facem un header pt tokenul de acces
    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);

    char *headers[] = {header_jwt};

    char *cookies[] = {cookie_user};

    // Trimitem token si cokie
    // Nu avem payload ca doar cerem ceva de la server
    message = compute_get_request(SERVER_IP, "/api/v1/tema/library/movies", NULL, headers, 1, cookies, 1);
    
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (response == NULL) {
        printf("ERROR: Nu s-a primit niciun raspuns de la server.\n");
        close_connection(sockfd);
        free(message);
        return;
    }

    if (strstr(response, "200 OK")) {
      printf("SUCCESS: Lista filmelor\n");

        // Extragem filmele din json
        char *json_start = strstr(response, "[");
        if (json_start) {
            JSON_Value *root_value = json_parse_string(json_start);
            JSON_Array *movies = json_value_get_array(root_value);
            movie_count = json_array_get_count(movies);
            // Pt fiecare film extragem ce campuri vrem sa afisam
            for (size_t i = 0; i < json_array_get_count(movies); i++) {
                JSON_Object *movie = json_array_get_object(movies, i);
                int id = (int) json_object_get_number(movie, "id");
                const char *title = json_object_get_string(movie, "title");
                movie_ids[i] = id;
                printf("#%d %s\n", id, title);
            }

            json_value_free(root_value);
        } else {
            printf("ERROR: Nu s-a putut extrage JSON-ul.\n");
        }
    } else {
        printf("ERROR: Eroare la obtinerea listei de filme.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}


void add_movie() {

    char title[256], description[512];
    int year;
    float rating;
    char *message, *response;
    int sockfd = -1;

    // Citire detalii film
    printf("title=");
    clear_stdin_buffer();
    if (fgets(title, sizeof(title), stdin) == NULL) {
        perror("fgets title");
        return;
    }
    title[strcspn(title, "\n")] = '\0';

    printf("year=");
    scanf("%d", &year);

    clear_stdin_buffer();
    printf("description=");
    if (fgets(description, sizeof(description), stdin) == NULL) {
        perror("fgets description");
        return;
    }
    description[strcspn(description, "\n")] = '\0';
    
    printf("rating=");
    scanf("%f", &rating);

    if (rating < 0.0 || rating > 10.0) {
        printf("ERROR: Rating invalid. Trebuie sa fie intre 0.0 si 10.0.\n");
        return;
    }

    // Ne populeaza campurile movie_count si movie_ids
    // Facem verificarea daca un film e deja in lista
    get_movies();
    for (int i = 0; i < movie_count; i++) {
        int movie_id = movie_ids[i];
        
        char url[256];
        // url dinamic
        snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", movie_id);

        sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

        char header_jwt[2048];
        snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
        char *headers[] = {header_jwt};

        char *cookies[] = {cookie_user};

        message = compute_get_request(SERVER_IP, url, NULL, headers, 1, cookies, 1);
        send_to_server(sockfd, message);
        response = receive_from_server(sockfd);
        close_connection(sockfd);
        free(message);

        if (response && strstr(response, "200 OK")) {
            char *json_start = strstr(response, "{");
            JSON_Value *root_value = json_parse_string(json_start);
            JSON_Object *root_object = json_value_get_object(root_value);

            const char *existing_title = json_object_get_string(root_object, "title");
            int existing_year = (int)json_object_get_number(root_object, "year");

            if (existing_title && strcmp(existing_title, title) == 0 && existing_year == year) {
                printf("INFO: Filmul '%s' din anul %d este deja adaugat. Nu va fi duplicat.\n", title, year);
                json_value_free(root_value);
                free(response);
                return;
            }

            json_value_free(root_value);
        }

        free(response);
    }

    // Daca filmul nu exista, continua cu adaugarea
    sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    // Creare JSON payload
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);
    json_object_set_number(root_object, "year", year);
    json_object_set_string(root_object, "description", description);
    json_object_set_number(root_object, "rating", rating);

    char *payload = json_serialize_to_string(root_value);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);

    char *headers[] = { header_jwt };

    message = compute_post_request(SERVER_IP, "/api/v1/tema/library/movies", 
                                   "application/json", &payload, 1, 
                                   headers, 1, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK") || strstr(response, "201 CREATED")) {
        printf("SUCCESS: Film adaugat cu succes!\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Fara acces la librarie. Folositi comanda get_access.\n");
    } else if (strstr(response, "401 UNAUTHORIZED")) {
        printf("ERROR: JWT token invalid sau lipseste. Folositi comanda get_access.\n");
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Date invalide sau incomplete pentru film.\n");
    } else {
    }

    json_free_serialized_string(payload);
    json_value_free(root_value);
    close_connection(sockfd);
    free(message);
    free(response);
}


void get_movie() {

    int movie_id;
    printf("id=");
    scanf("%d", &movie_id);

    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", movie_id);

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    char *cookies[] = {cookie_user};

    message = compute_get_request(SERVER_IP, url, NULL, headers, 1, cookies, 1);
    
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (response == NULL) {
        printf("ERROR: Nu s-a primit niciun raspuns de la server.\n");
        close_connection(sockfd);
        free(message);
        return;
    }

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Filmul obtinut cu succes!\n");

        char *json_start = strstr(response, "{");
        if (json_start) {
            JSON_Value *root_value = json_parse_string(json_start);
            if (root_value) {
                char *compact_json = json_serialize_to_string(root_value);

                printf("%s\n", compact_json);

                json_free_serialized_string(compact_json);
                json_value_free(root_value);
            } else {
                printf("ERROR: JSON invalid în raspuns.\n");
            }
        } else {
            printf("ERROR: Raspuns invalid de la server. \n");
        }
    } else {
        printf("ERROR: Filmul cu ID-ul %d nu a fost gasit.\n", movie_id);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void update_movie() {

    int film_id;
    printf("id=");
    scanf("%d", &film_id);

    char title[256], description[512];
    int year;
    float rating;
    clear_stdin_buffer();

    printf("title=");
    if (fgets(title, sizeof(title), stdin) == NULL) {
        perror("fgets title");
        return;
    }
    title[strcspn(title, "\n")] = '\0';

    printf("year=");
    scanf("%d", &year);

    clear_stdin_buffer();
    printf("description=");
    if (fgets(description, sizeof(description), stdin) == NULL) {
        perror("fgets description");
        return;
    }
    description[strcspn(description, "\n")] = '\0';
    
    printf("rating=");
    scanf("%f", &rating);

    if (rating < 0.0 || rating > 10.0) {
        printf("ERROR: Rating invalid. Trebuie sa fie intre 0.0 si 10.0.\n");
        return;
    }

    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", film_id);

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);
    json_object_set_number(root_object, "year", year);
    json_object_set_string(root_object, "description", description);
    json_object_set_number(root_object, "rating", rating);

    char *payload = json_serialize_to_string(root_value);

    char *cookies[] = {cookie_user};

    message = compute_put_request(SERVER_IP, url, "application/json", &payload, 1, headers, 1, cookies, 1);
    
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (response == NULL) {
        printf("ERROR: Nu s-a primit niciun raspuns de la server.\n");
        close_connection(sockfd);
        free(message);
        json_free_serialized_string(payload);
        json_value_free(root_value);
        return;
    }

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Filmul a fost actualizat cu succes!\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Fara acces la librarie. Folositi comanda get_access.\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Filmul cu ID-ul %d nu a fost gasit.\n", film_id);
    } else if (strstr(response, "400 BAD REQUEST")) {
        printf("ERROR: Date invalide sau incomplete pentru film.\n");
    } else {
        printf("ERROR: Eroare la actualizarea filmului.\n");
        printf("%s\n", response);
    }

    json_free_serialized_string(payload);
    json_value_free(root_value);
    close_connection(sockfd);
    free(message);
    free(response);
}

void delete_movie() {

    int film_id;
    printf("id=");
    scanf("%d", &film_id);


    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", film_id);

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    char *cookies[] = {cookie_user};

    message = compute_delete_request(SERVER_IP, url, headers, 1, cookies, 1);
    
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (response == NULL) {
        printf("ERROR: Nu s-a primit niciun raspuns de la server.\n");
        close_connection(sockfd);
        free(message);
        return;
    }

    if (strstr(response, "200 OK") || strstr(response, "204 NO CONTENT")) {
        printf("SUCCESS: Filmul cu ID-ul %d a fost sters cu succes!\n", film_id);
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Fara acces la librarie. Folositi comanda get_access.\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Filmul cu ID-ul %d nu a fost gasit.\n", film_id);
    } else {
        printf("ERROR: Eroare la stergerea filmului.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void get_collections() {

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};
    
    char *cookies[] = {cookie_user};

    message = compute_get_request(SERVER_IP, "/api/v1/tema/library/collections", 
                                    NULL, headers, 1, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Lista de colectii obtinuta cu succes!\n");

        char *json_start = strstr(response, "[");
        if (json_start == NULL) {
            printf("ERROR: Continut JSON invalid.\n");
            close_connection(sockfd);
            free(message);
            free(response);
            return;
        }

        JSON_Value *parsed_val = json_parse_string(json_start);
        JSON_Array *collection_arr = json_value_get_array(parsed_val);
        size_t num_collections = json_array_get_count(collection_arr);

        if (num_collections == 0) {
            printf("Nu exista colectii disponibile.\n");
        } else {
            for (int i = (int)num_collections - 1, idx = 1; i >= 0; i--, idx++) {
                JSON_Object *collection = json_array_get_object(collection_arr, i);
                const char *col_title = json_object_get_string(collection, "title");
                const char *owner = json_object_get_string(collection, "owner");
                int id = (int)json_object_get_number(collection, "id");

                printf("#%d\n", id);
                printf("  title: %s\n", col_title ? col_title : "N/A");
                printf("  owner: %s\n", owner ? owner : "N/A");
                printf("  id: %d\n\n", id);
            }
        }

        json_value_free(parsed_val);
    } else {
        printf("ERROR: Eroare la obtinerea listei de colectii.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}


void add_collection() {

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char title[256];
    int num_movies;

    getchar();
    printf("title=");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0';

    printf("num_movies=");
    scanf("%d", &num_movies);
    clear_stdin_buffer();

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);

    char *payload = json_serialize_to_string(root_value);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    char *cookies[] = {cookie_user};
    char *message = compute_post_request(SERVER_IP, "/api/v1/tema/library/collections",
                                         "application/json", &payload, 1, headers, 1, cookies, 1);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (!strstr(response, "201 CREATED")) {
        printf("ERROR: Eroare la crearea colectiei.\n");
        printf("%s\n", response);
        goto cleanup;
    }

    // Extragem id colectiei create
    char *json_start = strstr(response, "{");
    JSON_Value *resp_value = json_parse_string(json_start);
    JSON_Object *resp_object = json_value_get_object(resp_value);
    int collection_id = (int)json_object_get_number(resp_object, "id");

    printf("SUCCESS: Colectie creata cu ID: %d\n", collection_id);

    // Adauga filme în colectie
    for (int i = 0; i < num_movies; i++) {
        printf("movie_id[%d]=", i);
        int movie_id;
        scanf("%d", &movie_id);

        char url[256];
        snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d/movies", collection_id);

        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        json_object_set_number(root_object, "id", movie_id);

        char *payload = json_serialize_to_string(root_value);
        char *movie_message = compute_post_request(SERVER_IP, url, "application/json",
                                                   &payload, 1, headers, 1, NULL, 0);
        send_to_server(sockfd, movie_message);
        char *movie_response = receive_from_server(sockfd);

        if (strstr(movie_response, "201 CREATED")) {
            printf("SUCCESS: Filmul cu ID %d adaugat în colectie.\n", movie_id);
        } else {
            printf("ERROR:Nu exista filmul cu ID-ul %d \n", movie_id);
        }

        free(payload);
        free(movie_message);
        free(movie_response);
        json_value_free(root_value);
    }

cleanup:
    free(message);
    free(response);
    json_free_serialized_string(payload);
    json_value_free(root_value);
    json_value_free(resp_value);
    close_connection(sockfd);
}

void add_movie_to_collection() {

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    int collection_ind, movie_id;


    printf("collection_id=\n");
    scanf("%d", &collection_ind);

    printf("movie_id=\n");
    scanf("%d", &movie_id);

    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d/movies", collection_ind);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_number(root_object, "id", movie_id);

    char *payload = json_serialize_to_string(root_value);
    char *message = compute_post_request(SERVER_IP, url, "application/json", &payload, 1, headers, 1, NULL, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    free(message);
    free(response);
    json_free_serialized_string(payload);
    json_value_free(root_value);
    close_connection(sockfd);

}

void get_collection() {

    int collection_id;
    printf("id=");
    scanf("%d", &collection_id);


    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d", collection_id);

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    char *cookies[] = {cookie_user};
    message = compute_get_request(SERVER_IP, url, NULL, headers, 1, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);


    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Detalii colectie obtinute cu succes!\n");
        char *json_body = strstr(response, "{");

        JSON_Value *json_val = json_parse_string(json_body);
        JSON_Object *collection_data = json_value_get_object(json_val);

        const char *collection_name = json_object_get_string(collection_data, "title");
        const char *created_by = json_object_get_string(collection_data, "owner");
        JSON_Array *film_list = json_object_get_array(collection_data, "movies");

        printf("SUCCESS: Informatii colectie\n");
        printf("title: %s\n", collection_name);
        printf("owner: %s\n", created_by);

        size_t total_films = json_array_get_count(film_list);
        for (size_t i = 0; i < total_films; i++) {
            JSON_Object *film = json_array_get_object(film_list, i);
            int fid = (int)json_object_get_number(film, "id");
            const char *fname = json_object_get_string(film, "title");
            printf("#%d: %s\n", fid, fname);
        }

        json_value_free(json_val);;
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Fara acces la librarie. Folositi comanda get_access.\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Colectia cu ID-ul %d nu a fost gasita.\n", collection_id);
    } else {
        printf("ERROR: Eroare la obtinerea detaliilor colectiei.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void delete_collection() {

    char in[256];
    int id;
    char *eptr;
    int c;

    while ((c = getchar()) != '\n' && c != EOF) {
        ;
    }

    printf("id=");
    if (!fgets(in, sizeof(in), stdin)) {
        fprintf(stderr, "ERROR: Eroare input.\n");
        return;
    }
    in[strcspn(in, "\n")] = '\0';

    if (in[0] == '\0') {
        printf("ERROR: Nu e bun numarul.\n");
        return;
    }

    id = strtol(in, &eptr, 10);

    if (id < 0) {
        printf("ERROR: ID-ul negativ.\n");
        return;
    }

    if (*eptr != '\0' || eptr == in ) {
        printf("ERROR: Nu este un numar valid.\n");
        return;
    }



    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d", id);

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};
    
    char *cookies[] = {cookie_user};

    message = compute_delete_request(SERVER_IP, url, headers, 1, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK") || strstr(response, "204 NO CONTENT")) {
        printf("SUCCESS: Colectie stearsa cu succes!\n");
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Nu aveti permisiuni.\n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Colectia cu ID-ul %d nu a fost gasita.\n", id);
    } else {
        printf("ERROR: Eroare la stergerea colectiei.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}

void delete_movie_from_collection() {

    int collection_id, movie_id;
    printf("collection_id=");
    scanf("%d", &collection_id);

    if (collection_id == -1) {
        printf("ERROR: Colectia nu a fost gasita.\n");
        return;
    }

    printf("movie_id=");
    scanf("%d", &movie_id);

    char url[256];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d/movies/%d", 
                collection_id, movie_id);

    char *message, *response;
    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);

    char header_jwt[2048];
    snprintf(header_jwt, sizeof(header_jwt), "Authorization: Bearer %s", jwt_token);
    char *headers[] = {header_jwt};

    char *cookies[] = {cookie_user};

    message = compute_delete_request(SERVER_IP, url, headers, 1, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strstr(response, "200 OK") || strstr(response, "204 NO CONTENT")) {
        printf("SUCCESS: Filmul cu ID %d a fost sters din colectie!\n", movie_id);
    } else if (strstr(response, "403 FORBIDDEN")) {
        printf("ERROR: Nu aveti permisiuni. \n");
    } else if (strstr(response, "404 NOT FOUND")) {
        printf("ERROR: Colectia sau filmul cu ID-ul %d nu a fost gasit în colectie.\n", movie_id);
    } else {
        printf("ERROR: Eroare la stergerea filmului din colectie.\n");
        printf("%s\n", response);
    }

    close_connection(sockfd);
    free(message);
    free(response);
}


int main() {
    char command[50];

    while (1) {
        scanf("%s", command);

        if (strcmp(command, "login_admin") == 0) {
            login_admin();
        } else if (strcmp(command, "get_users") == 0) {
            get_users();
        } else if (strcmp(command, "delete_user") == 0) {
            delete_user();
        } else if (strcmp(command, "logout_admin") == 0) {
            logout_admin();
        } else if (strcmp(command, "login") == 0) {
            login();
        } else if (strcmp(command, "get_access") == 0) {
            get_access();
        } else if (strcmp(command, "get_movies") == 0) {
            get_movies();
        } else if (strcmp(command, "logout") == 0) {
            logout();
        } else if (strcmp(command, "add_movie") == 0) {
            add_movie();
        } else if (strcmp(command, "get_movie") == 0) {
            get_movie();
        } else if (strcmp(command, "update_movie") == 0) {
            update_movie();
        } else if (strcmp(command, "get_collection") == 0) {
            get_collection();
        } else if (strcmp(command, "get_collections") == 0) {
            get_collections();
        } else if (strcmp(command, "delete_movie") == 0) {
            delete_movie();
        } else if (strcmp(command, "add_collection") == 0) {
            add_collection();
        } else if (strcmp(command, "delete_movie_from_collection") == 0) {
            delete_movie_from_collection();
        } else if (strcmp(command, "delete_collection") == 0) {
            delete_collection();
        } else if (strcmp(command, "add_movie_to_collection") == 0) {
            add_movie_to_collection();
        } else if (strcmp(command, "add_user") == 0) {
            add_user();
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting...\n");
            break;
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}