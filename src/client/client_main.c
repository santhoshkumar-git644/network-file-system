#include "common.h"
#include "logger.h"
#include "client_handler.h"

int connect_to_nm(const char* nm_ip, int nm_port) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_message(LOG_ERROR, "Socket creation error");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(nm_port);
    
    if (inet_pton(AF_INET, nm_ip, &serv_addr.sin_addr) <= 0) {
        log_message(LOG_ERROR, "Invalid address/ Address not supported");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_message(LOG_ERROR, "Connection to NM Failed");
        return -1;
    }
    
    return sock;
}

void connect_to_ss_and_read(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_message(LOG_ERROR, "SS Socket creation error");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_message(LOG_ERROR, "Connection to SS Failed");
        return;
    }
    
    // Send READ request to SS
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_READ;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    // Receive file content
    char buffer[MAX_BUFFER_SIZE];
    int bytes;
    printf("--- File Content (%s) ---\n", filename);
    while ((bytes = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "EOF")) { // End marker
            break;
        }
    }
    printf("\n--- End of File ---\n");
    close(sock);
}

int main(int argc, char *argv[]) {
    init_logger(NULL); // Client logs to stdout by default
    
    char username[MAX_USERNAME];
    printf("Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        log_message(LOG_ERROR, "Failed to read username");
        return 1;
    }
    username[strcspn(username, "\n")] = 0; // Remove newline
    
    int nm_sock = connect_to_nm("127.0.0.1", 8082); // NM_CLIENT_PORT
    if (nm_sock < 0) {
        return 1;
    }
    
    log_message(LOG_INFO, "Client started and connected to NM for user: %s", username);

    // Main loop for commands
    char command_buffer[MAX_BUFFER_SIZE];
    while (1) {
        printf("Client> ");
        if (fgets(command_buffer, sizeof(command_buffer), stdin) == NULL) {
            break; // EOF or error
        }
        command_buffer[strcspn(command_buffer, "\n")] = 0;
        
        if (strcmp(command_buffer, "EXIT") == 0) {
            break;
        }
        
        // Parse command
        ClientCommand cmd;
        memset(&cmd, 0, sizeof(ClientCommand));
        
        char *token = strtok(command_buffer, " ");
        if (token == NULL) continue;
        
        if (strcmp(token, "VIEW") == 0) {
            cmd.type = CMD_VIEW;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
        } else if (strcmp(token, "LIST") == 0) {
            cmd.type = CMD_LIST_USERS;
        } else if (strcmp(token, "READ") == 0) {
            cmd.type = CMD_READ;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: READ <filename>\n");
                continue;
            }
        } else {
            cmd.type = CMD_UNKNOWN;
            log_message(LOG_WARN, "Unknown command or not yet implemented");
            continue;
        }
        
        send(nm_sock, &cmd, sizeof(ClientCommand), 0);
        
        // Wait for response
        char response[MAX_BUFFER_SIZE];
        int bytes = recv(nm_sock, response, sizeof(response) - 1, 0);
        if (bytes > 0) {
            response[bytes] = '\0';
            
            if (strncmp(response, "SS_INFO", 7) == 0) {
                char ip[INET_ADDRSTRLEN];
                int port;
                if (sscanf(response, "SS_INFO %s %d", ip, &port) == 2) {
                    connect_to_ss_and_read(ip, port, cmd.arg1);
                } else {
                    printf("ERROR: Malformed SS_INFO response\n");
                }
            } else {
                printf("%s\n", response);
            }
        }
    }

    close(nm_sock);
    close_logger();
    return 0;
}
