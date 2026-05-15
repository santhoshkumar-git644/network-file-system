#include "client_handler.h"
#include "logger.h"
#include "nm_hashmap.h"

void* handle_client_connection(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    ClientCommand cmd;
    
    while (1) {
        int bytes_read = recv(client_socket, &cmd, sizeof(ClientCommand), 0);
        if (bytes_read <= 0) {
            log_message(LOG_INFO, "Client disconnected");
            break;
        }
        
        char response[MAX_BUFFER_SIZE];
        memset(response, 0, sizeof(response));
        
        if (cmd.type == CMD_VIEW) {
            log_message(LOG_INFO, "Received VIEW command");
            // Placeholder: In reality, we'd list files the user has access to.
            // For now, let's just return a static list or ack.
            strcpy(response, "--> file1.txt\n--> file2.txt");
        } else if (cmd.type == CMD_LIST_USERS) {
            log_message(LOG_INFO, "Received LIST command");
            // Placeholder: List users
            strcpy(response, "--> user1\n--> user2");
        } else {
            log_message(LOG_WARN, "Received UNKNOWN command");
            strcpy(response, "ERROR: Unknown command");
        }
        
        send(client_socket, response, strlen(response), 0);
    }
    
    close(client_socket);
    return NULL;
}
