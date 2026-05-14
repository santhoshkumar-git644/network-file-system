#include "common.h"
#include "logger.h"

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

int main(int argc, char *argv[]) {
    init_logger(NULL); // Client logs to stdout by default
    
    char username[MAX_USERNAME];
    printf("Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        log_message(LOG_ERROR, "Failed to read username");
        return 1;
    }
    username[strcspn(username, "\n")] = 0; // Remove newline
    
    int nm_sock = connect_to_nm("127.0.0.1", NM_PORT);
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
        
        // Parse and send command to NM (to be implemented)
        log_message(LOG_INFO, "Command entered: %s", command_buffer);
        // send(nm_sock, command_buffer, strlen(command_buffer), 0);
    }

    close(nm_sock);
    close_logger();
    return 0;
}
