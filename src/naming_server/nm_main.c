#include "nm_core.h"

int main(int argc, char *argv[]) {
    init_logger("nm.log");
    log_message(LOG_INFO, "Naming Server started.");
    
    init_nm_state();

    // Setup socket to listen for SS and Client connections
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

    log_message(LOG_INFO, "Naming Server listening on port %d", NM_PORT);

    while (1) {
        int *new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            log_message(LOG_ERROR, "Accept failed");
            free(new_socket);
            continue;
        }
        
        log_message(LOG_INFO, "New connection accepted");
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_ss_connection, (void*)new_socket) < 0) {
            log_message(LOG_ERROR, "Could not create thread");
            free(new_socket);
        }
        
        pthread_detach(thread_id);
    }

    close_logger();
    return 0;
}
