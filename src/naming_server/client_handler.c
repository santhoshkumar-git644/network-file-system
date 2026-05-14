#include "client_handler.h"
#include "logger.h"

void* handle_client_connection(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    char buffer[MAX_BUFFER_SIZE];
    
    while (1) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            log_message(LOG_INFO, "Client disconnected");
            break;
        }
        
        buffer[bytes_read] = '\0';
        log_message(LOG_INFO, "Received from client: %s", buffer);
        
        // Simple echo for now, full parsing logic comes later
        send(client_socket, "ACK", 3, 0);
    }
    
    close(client_socket);
    return NULL;
}
