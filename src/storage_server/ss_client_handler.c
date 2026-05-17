#include "ss_client_handler.h"
#include "logger.h"

void* ss_handle_client_connection(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    ClientCommand cmd;
    int bytes_read = recv(client_socket, &cmd, sizeof(ClientCommand), 0);
    if (bytes_read > 0) {
        if (cmd.type == CMD_READ) {
            log_message(LOG_INFO, "Client requested READ for file: %s", cmd.arg1);
            FILE *fp = fopen(cmd.arg1, "r");
            if (fp) {
                char buffer[MAX_BUFFER_SIZE];
                while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                fclose(fp);
            } else {
                send(client_socket, "ERROR: File not found on SS\n", 28, 0);
            }
            send(client_socket, "EOF\n", 4, 0);
        } else {
            log_message(LOG_WARN, "Unsupported command received by SS from client");
        }
    }
    
    close(client_socket);
    return NULL;
}

void* ss_client_listener_thread(void* arg) {
    int client_port = *((int*)arg);
    free(arg);
    
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_message(LOG_ERROR, "SS Client Socket creation failed");
        return NULL;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(client_port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message(LOG_ERROR, "SS Bind failed on client port %d", client_port);
        return NULL;
    }

    if (listen(server_fd, 10) < 0) {
        log_message(LOG_ERROR, "SS Client Listen failed");
        return NULL;
    }

    log_message(LOG_INFO, "Storage Server listening for Clients on port %d", client_port);

    while (1) {
        int *new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            log_message(LOG_ERROR, "SS Client Accept failed");
            free(new_socket);
            continue;
        }
        
        log_message(LOG_INFO, "New Client connection to SS accepted");
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, ss_handle_client_connection, (void*)new_socket) < 0) {
            log_message(LOG_ERROR, "Could not create SS client thread");
            free(new_socket);
        }
        pthread_detach(thread_id);
    }
    return NULL;
}
