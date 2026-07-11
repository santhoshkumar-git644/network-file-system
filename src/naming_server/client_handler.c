#include "client_handler.h"
#include "logger.h"
#include "nm_core.h"
#include "nm_hashmap.h"
#include "nm_cache.h"
#include "nm_replication.h"
#include "nm_users.h"

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
            log_message(LOG_INFO, "Received VIEW command for user: %s", cmd.username);
            // Return all files the requesting user has access to
            // (simple approach: search all hashmap entries)
            char search_results[MAX_BUFFER_SIZE];
            hashmap_search("", search_results, sizeof(search_results)); // empty string matches all
            strncpy(response, search_results, MAX_BUFFER_SIZE - 1);
        } else if (cmd.type == CMD_LIST_USERS) {
            log_message(LOG_INFO, "Received LIST command");
            // List registered users from nm_users
            char user_list[MAX_BUFFER_SIZE];
            list_users(user_list, sizeof(user_list));
            strncpy(response, user_list, MAX_BUFFER_SIZE - 1);
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
            log_message(LOG_INFO, "Received %s command for target: %s from user: %s", cmd_name, cmd.arg1, cmd.username);
            
            // Basic authorization check
            if (cmd.type == CMD_READ || cmd.type == CMD_WRITE || cmd.type == CMD_DELETE) {
                if (!has_access(cmd.username, cmd.arg1)) {
                    // Fix #11: send error and break loop instead of force-closing socket
                    strcpy(response, "ERROR: Access denied");
                    send(client_socket, response, strlen(response), 0);
                    continue; // Do not process further, but keep connection open
                }
            }
            
            int ss_id = cache_get(cmd.arg1);
            if (ss_id >= 0) {
                log_message(LOG_INFO, "Cache hit for %s", cmd.arg1);
            } else {
                ss_id = hashmap_lookup(cmd.arg1);
                if (ss_id >= 0) {
                    log_message(LOG_INFO, "Cache miss for %s. Adding to cache.", cmd.arg1);
                    cache_put(cmd.arg1, ss_id);
                }
            }
            
            if (ss_id >= 0 && ss_list[ss_id].is_active) {
                sprintf(response, "SS_INFO %s %d", ss_list[ss_id].info.ip, ss_list[ss_id].info.client_port);
                
                // If it's a delete, we should ideally remove it from the hashmap after successful deletion,
                // but for simplicity we can remove it immediately from NM perspective.
                if (cmd.type == CMD_DELETE) {
                    hashmap_delete(cmd.arg1);
                    cache_invalidate(cmd.arg1);
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
                    cache_put(cmd.arg1, selected_ss);
                    
                    // Simple asynchronous replication trigger (in real system, would wait for SS confirmation)
                    trigger_replication(cmd.arg1, selected_ss);
                    
                    // Grant access to creator
                    grant_access(cmd.username, cmd.arg1);
                    
                    sprintf(response, "SS_INFO %s %d", ss_list[selected_ss].info.ip, ss_list[selected_ss].info.client_port);
                } else {
                    strcpy(response, "ERROR: No Storage Servers available");
                }
            }
        } else if (cmd.type == CMD_SEARCH) {
            log_message(LOG_INFO, "Received SEARCH command for substring: %s", cmd.arg1);
            char search_results[MAX_BUFFER_SIZE];
            hashmap_search(cmd.arg1, search_results, sizeof(search_results));
            strncpy(response, search_results, MAX_BUFFER_SIZE - 1);
        } else if (cmd.type == CMD_ADD_USER) {
            log_message(LOG_INFO, "Received ADDUSER command for user: %s", cmd.arg1);
            if (add_user(cmd.arg1) == 1) {
                strcpy(response, "USER_ADDED");
            } else {
                strcpy(response, "ERROR: User exists or max users reached");
            }
        } else if (cmd.type == CMD_LOGIN) {
            log_message(LOG_INFO, "Received LOGIN command for user: %s", cmd.arg1);
            if (user_exists(cmd.arg1)) {
                strcpy(response, "LOGIN_SUCCESS");
            } else {
                strcpy(response, "ERROR: User not found");
            }
        } else if (cmd.type == CMD_GRANT_ACCESS) {
            log_message(LOG_INFO, "Received GRANT command for file: %s to user: %s", cmd.arg1, cmd.arg2);
            if (grant_access(cmd.arg2, cmd.arg1)) {
                strcpy(response, "ACCESS_GRANTED");
            } else {
                strcpy(response, "ERROR: Could not grant access");
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
