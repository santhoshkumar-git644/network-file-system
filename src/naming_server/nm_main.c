#include "nm_core.h"
#include "client_handler.h"

#define NM_CLIENT_PORT 8082

void* client_listener_thread(void* arg) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_message(LOG_ERROR, "Client Socket creation failed");
        return NULL;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(NM_CLIENT_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message(LOG_ERROR, "Bind failed on client port %d", NM_CLIENT_PORT);
        return NULL;
    }

    if (listen(server_fd, 10) < 0) {
        log_message(LOG_ERROR, "Client Listen failed");
        return NULL;
    }

    log_message(LOG_INFO, "Naming Server listening for Clients on port %d", NM_CLIENT_PORT);

    while (1) {
        int *new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            log_message(LOG_ERROR, "Client Accept failed");
            free(new_socket);
            continue;
        }
        
        log_message(LOG_INFO, "New Client connection accepted");
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_connection, (void*)new_socket) < 0) {
            log_message(LOG_ERROR, "Could not create client thread");
            free(new_socket);
        }
        pthread_detach(thread_id);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    init_logger("nm.log");
    log_message(LOG_INFO, "Naming Server started.");
    
    init_nm_state();

    // Start client listener in a background thread
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, client_listener_thread, NULL);

    // Setup socket to listen for SS connections
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_message(LOG_ERROR, "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        log_message(LOG_ERROR, "Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(NM_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message(LOG_ERROR, "Bind failed on port %d", NM_PORT);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        log_message(LOG_ERROR, "Listen failed");
        exit(EXIT_FAILURE);
    }

    log_message(LOG_INFO, "Naming Server listening for SS on port %d", NM_PORT);

    while (1) {
        int *new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            log_message(LOG_ERROR, "Accept failed");
            free(new_socket);
            continue;
        }
        
        log_message(LOG_INFO, "New SS connection accepted");
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_ss_connection, (void*)new_socket) < 0) {
            log_message(LOG_ERROR, "Could not create SS thread");
            free(new_socket);
        }
        
        pthread_detach(thread_id);
    }

    close_logger();
    return 0;
}
