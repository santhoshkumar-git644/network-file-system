#include "common.h"
#include "logger.h"

int main(int argc, char *argv[]) {
    init_logger(NULL); // Client logs to stdout by default
    
    char username[MAX_USERNAME];
    printf("Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        log_message(LOG_ERROR, "Failed to read username");
        return 1;
    }
    username[strcspn(username, "\n")] = 0; // Remove newline
    
    log_message(LOG_INFO, "Client started for user: %s", username);

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
    }

    close_logger();
    return 0;
}
