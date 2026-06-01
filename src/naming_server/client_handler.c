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
        } else if (cmd.type == CMD_READ || cmd.type == CMD_WRITE || cmd.type == CMD_UNDO || 
                   cmd.type == CMD_DELETE || cmd.type == CMD_INFO || 
                   cmd.type == CMD_STREAM || cmd.type == CMD_EXEC || cmd.type == CMD_LIST_DIR) {
            const char* cmd_name = (cmd.type == CMD_READ) ? "READ" : 
                                   (cmd.type == CMD_WRITE) ? "WRITE" : 
                                   (cmd.type == CMD_UNDO) ? "UNDO" : 
                                   (cmd.type == CMD_DELETE) ? "DELETE" : 
                                   (cmd.type == CMD_STREAM) ? "STREAM" : 
                                   (cmd.type == CMD_EXEC) ? "EXEC" : 
                                   (cmd.type == CMD_LIST_DIR) ? "LSDIR" : "INFO";
            log_message(LOG_INFO, "Received %s command for target: %s", cmd_name, cmd.arg1);
            int ss_id = hashmap_lookup(cmd.arg1);
            if (ss_id >= 0 && ss_list[ss_id].is_active) {
                sprintf(response, "SS_INFO %s %d", ss_list[ss_id].info.ip, ss_list[ss_id].info.client_port);
                
                // If it's a delete, we should ideally remove it from the hashmap after successful deletion,
                // but for simplicity we can remove it immediately from NM perspective.
                if (cmd.type == CMD_DELETE) {
                    hashmap_delete(cmd.arg1);
                }
            } else {
                strcpy(response, "ERROR: File/Directory not found or SS down");
            }
        } else if (cmd.type == CMD_CREATE || cmd.type == CMD_CREATE_DIR) {
            const char* cmd_name = (cmd.type == CMD_CREATE) ? "CREATE" : "MKDIR";
            log_message(LOG_INFO, "Received %s command for target: %s", cmd_name, cmd.arg1);
            if (hashmap_lookup(cmd.arg1) >= 0) {
                strcpy(response, "ERROR: File/Directory already exists");
            } else {
                static int last_selected_ss = -1;
                int selected_ss = -1;
                
                // Round robin selection
                for (int i = 1; i <= MAX_STORAGE_SERVERS; i++) {
                    int idx = (last_selected_ss + i) % MAX_STORAGE_SERVERS;
                    if (ss_list[idx].is_active) {
                        selected_ss = idx;
                        last_selected_ss = idx;
                        break;
                    }
                }
                
                if (selected_ss >= 0) {
                    // Tell client to connect to SS and CREATE (Client directly creates on SS)
                    // Alternatively NM can tell SS. We'll return SS_INFO to Client so Client creates it.
                    hashmap_insert(cmd.arg1, selected_ss);
                    sprintf(response, "SS_INFO %s %d", ss_list[selected_ss].info.ip, ss_list[selected_ss].info.client_port);
                } else {
                    strcpy(response, "ERROR: No Storage Servers available");
                }
            }
        } else {
            log_message(LOG_WARN, "Received UNKNOWN command");
            strcpy(response, "ERROR: Unknown command");
        }
        
        send(client_socket, response, strlen(response), 0);
    }
    
    close(client_socket);
    return NULL;
}
