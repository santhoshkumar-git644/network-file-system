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

void connect_to_ss_and_write(const char* ip, int port, ClientCommand* initial_cmd) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_message(LOG_ERROR, "SS Socket creation error for WRITE");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_message(LOG_ERROR, "Connection to SS Failed for WRITE");
        return;
    }
    
    // Send initial WRITE request
    send(sock, initial_cmd, sizeof(ClientCommand), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
        if (strstr(buffer, "ERROR")) {
            close(sock);
            return;
        }
    }
    
    // Interactive write loop until ETIRW
    while (1) {
        printf("Client (WRITE %s)> ", initial_cmd->arg1);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;
        
        send(sock, buffer, strlen(buffer), 0);
        
        if (strcmp(buffer, "ETIRW") == 0) {
            bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf("%s\n", buffer);
            }
            break;
        }
        
        // Wait for ack/result
        bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("%s\n", buffer);
        }
    }
    close(sock);
}

void connect_to_ss_and_undo(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_message(LOG_ERROR, "SS Socket creation error for UNDO");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_message(LOG_ERROR, "Connection to SS Failed for UNDO");
        return;
    }
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_UNDO;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
}

void connect_to_ss_and_create(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_message(LOG_ERROR, "SS Socket creation error for CREATE");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_message(LOG_ERROR, "Connection to SS Failed for CREATE");
        return;
    }
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_CREATE;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
}

void connect_to_ss_and_delete(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return;
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_DELETE;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
}

void connect_to_ss_and_info(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return;
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_INFO;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
}

void connect_to_ss_and_stream(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return;
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_STREAM;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes;
    printf("--- Streaming File Content ---\n");
    while ((bytes = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "EOF")) break;
    }
    printf("\n--- End of Stream ---\n");
    close(sock);
}

void connect_to_ss_and_exec(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return;
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_EXEC;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
}

void connect_to_ss_and_mkdir(const char* ip, int port, const char* dirname) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return;
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_CREATE_DIR;
    strncpy(cmd.arg1, dirname, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
}

void connect_to_ss_and_lsdir(const char* ip, int port, const char* dirname) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return;
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_LIST_DIR;
    strncpy(cmd.arg1, dirname, MAX_FILENAME);
    send(sock, &cmd, sizeof(cmd), 0);
    
    char buffer[MAX_BUFFER_SIZE];
    int bytes;
    printf("--- Directory Listing (%s) ---\n", dirname);
    while ((bytes = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "EOF")) break;
    }
    printf("\n--- End of Listing ---\n");
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
        strncpy(cmd.username, username, sizeof(cmd.username) - 1);
        
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
        } else if (strcmp(token, "WRITE") == 0) {
            cmd.type = CMD_WRITE;
            char *arg1 = strtok(NULL, " ");
            char *arg2 = strtok(NULL, " ");
            char *arg3 = strtok(NULL, " "); // Optional SYNC/ASYNC
            
            if (arg1 && arg2) {
                strncpy(cmd.arg1, arg1, sizeof(cmd.arg1) - 1);
                strncpy(cmd.arg2, arg2, sizeof(cmd.arg2) - 1); // sentence number
                
                if (arg3 && strcmp(arg3, "ASYNC") == 0) {
                    cmd.is_sync = 0;
                } else {
                    cmd.is_sync = 1; // Default to synchronous
                }
            } else {
                printf("Usage: WRITE <filename> <sentence_number> [SYNC|ASYNC]\n");
                continue;
            }
            // For now, simple interaction. In a full implementation, the client would enter a write loop until ETIRW.
        } else if (strcmp(token, "CREATE") == 0) {
            cmd.type = CMD_CREATE;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: CREATE <filename>\n");
                continue;
            }
        } else if (strcmp(token, "DELETE") == 0) {
            cmd.type = CMD_DELETE;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: DELETE <filename>\n");
                continue;
            }
        } else if (strcmp(token, "INFO") == 0) {
            cmd.type = CMD_INFO;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: INFO <filename>\n");
                continue;
            }
        } else if (strcmp(token, "STREAM") == 0) {
            cmd.type = CMD_STREAM;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: STREAM <filename>\n");
                continue;
            }
        } else if (strcmp(token, "EXEC") == 0) {
            cmd.type = CMD_EXEC;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: EXEC <filename>\n");
                continue;
            }
        } else if (strcmp(token, "MKDIR") == 0) {
            cmd.type = CMD_CREATE_DIR;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: MKDIR <dirname>\n");
                continue;
            }
        } else if (strcmp(token, "LSDIR") == 0) {
            cmd.type = CMD_LIST_DIR;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: LSDIR <dirname>\n");
                continue;
            }
        } else if (strcmp(token, "SEARCH") == 0) {
            cmd.type = CMD_SEARCH;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: SEARCH <substring>\n");
                continue;
            }
        } else if (strcmp(token, "ADDUSER") == 0) {
            cmd.type = CMD_ADD_USER;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: ADDUSER <username>\n");
                continue;
            }
        } else if (strcmp(token, "LOGIN") == 0) {
            cmd.type = CMD_LOGIN;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: LOGIN <username>\n");
                continue;
            }
        } else if (strcmp(token, "GRANT") == 0) {
            cmd.type = CMD_GRANT_ACCESS;
            char *arg1 = strtok(NULL, " ");
            char *arg2 = strtok(NULL, " ");
            if (arg1 && arg2) {
                strncpy(cmd.arg1, arg1, sizeof(cmd.arg1) - 1); // file
                strncpy(cmd.arg2, arg2, sizeof(cmd.arg2) - 1); // user
            } else {
                printf("Usage: GRANT <file> <username>\n");
                continue;
            }
        } else if (strcmp(token, "UNDO") == 0) {
            cmd.type = CMD_UNDO;
            char *arg = strtok(NULL, " ");
            if (arg) strncpy(cmd.arg1, arg, sizeof(cmd.arg1) - 1);
            else {
                printf("Usage: UNDO <filename>\n");
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
                    if (cmd.type == CMD_READ) {
                        connect_to_ss_and_read(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_WRITE) {
                        connect_to_ss_and_write(ip, port, &cmd);
                    } else if (cmd.type == CMD_UNDO) {
                        connect_to_ss_and_undo(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_CREATE) {
                        connect_to_ss_and_create(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_DELETE) {
                        connect_to_ss_and_delete(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_INFO) {
                        connect_to_ss_and_info(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_STREAM) {
                        connect_to_ss_and_stream(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_EXEC) {
                        connect_to_ss_and_exec(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_CREATE_DIR) {
                        connect_to_ss_and_mkdir(ip, port, cmd.arg1);
                    } else if (cmd.type == CMD_LIST_DIR) {
                        connect_to_ss_and_lsdir(ip, port, cmd.arg1);
                    }
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
