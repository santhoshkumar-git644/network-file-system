#include "ss_client_handler.h"
#include "logger.h"
#include "sentence_parser.h"
#include "ss_file_manager.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#endif

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
                send(client_socket, "EOF\n", 4, 0); // End marker only on success
            } else {
                char err[] = "ERROR: File not found on SS\nEOF\n";
                send(client_socket, err, strlen(err), 0); // Send EOF too so client doesn't hang
            }
        } else if (cmd.type == CMD_WRITE) {
            log_message(LOG_INFO, "Client requested WRITE for file: %s (Sentence: %s)", cmd.arg1, cmd.arg2);
            int sentence_index = atoi(cmd.arg2);
            
            SSFileLock *fl = get_or_create_file_lock(cmd.arg1);
            if (!fl) {
                send(client_socket, "ERROR: Could not get file lock structure\n", 41, 0);
            } else {
                log_message(LOG_INFO, "Locking sentence %d for file %s (Sync: %d)", sentence_index, cmd.arg1, cmd.is_sync);
                
                if (acquire_write_lock(fl, sentence_index, cmd.is_sync) != 0) {
                    send(client_socket, "ERROR: Could not acquire write lock (File busy)\n", 48, 0);
                    // No need to unlock since we didn't acquire it
                } else {
                    // Read entire file into memory for now
                    FILE *fp = fopen(cmd.arg1, "r");
            if (!fp) {
                send(client_socket, "ERROR: File not found on SS\n", 28, 0);
            } else {
                // Create backup for UNDO
                char backup_name[MAX_FILENAME];
                snprintf(backup_name, sizeof(backup_name), ".%s.bak", cmd.arg1);
                
                FILE *backup_fp = fopen(backup_name, "w");
                if (backup_fp) {
                    char temp_buf[4096];
                    size_t bytes_read;
                    while ((bytes_read = fread(temp_buf, 1, sizeof(temp_buf), fp)) > 0) {
                        fwrite(temp_buf, 1, bytes_read, backup_fp);
                    }
                    fclose(backup_fp);
                    fseek(fp, 0, SEEK_SET); // Reset pointer after copying
                } else {
                    log_message(LOG_WARN, "Failed to create backup for UNDO");
                }
                
                fseek(fp, 0, SEEK_END);
                long fsize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                
                char *file_content = malloc(fsize + 1);
                fread(file_content, 1, fsize, fp);
                file_content[fsize] = 0;
                fclose(fp);
                
                ParsedFile parsed;
                parse_file_content(file_content, &parsed);
                free(file_content);
                
                if (sentence_index < 0 || sentence_index > parsed.sentence_count) {
                    send(client_socket, "ERROR: Sentence index out of range\n", 35, 0);
                    release_write_lock(fl, sentence_index); // Fix #3: must release lock on early exit
                } else {
                    send(client_socket, "ACK: Write session started", 26, 0);
                    char buffer[MAX_BUFFER_SIZE];
                    while (1) {
                        int b = recv(client_socket, buffer, sizeof(buffer)-1, 0);
                        if (b <= 0) break;
                        buffer[b] = '\0';
                        
                        if (strcmp(buffer, "ETIRW") == 0) {
                            send(client_socket, "Write Successful!", 17, 0);
                            break;
                        }
                        
                        // Parse <word_index> <content>
                        char* word_idx_str = strtok(buffer, " ");
                        char* new_word = strtok(NULL, "");
                        
                        if (word_idx_str && new_word) {
                            int word_idx = atoi(word_idx_str);
                            Sentence* s = &parsed.sentences[sentence_index];
                            
                            // Insert word (shift others), guard against overflow
                            if (word_idx >= 0 && word_idx <= s->word_count && s->word_count < MAX_WORDS_PER_SENTENCE) {
                                for (int i = s->word_count; i > word_idx; i--) {
                                    strncpy(s->words[i], s->words[i-1], MAX_WORD_LEN - 1);
                                    s->words[i][MAX_WORD_LEN - 1] = '\0';
                                }
                                strncpy(s->words[word_idx], new_word, MAX_WORD_LEN - 1);
                                s->words[word_idx][MAX_WORD_LEN - 1] = '\0';
                                s->word_count++;
                                send(client_socket, "ACK: Word inserted", 18, 0);
                            } else {
                                send(client_socket, "ERROR: Word index out of range", 30, 0);
                            }
                        } else {
                            send(client_socket, "ERROR: Invalid format", 21, 0);
                        }
                    }
                    
                    // Write back to file
                    char *new_content = malloc(MAX_BUFFER_SIZE * 10);
                    reconstruct_file_content(&parsed, new_content);
                    fp = fopen(cmd.arg1, "w");
                    if (fp) {
                        fputs(new_content, fp);
                        fclose(fp);
                    }
                    free(new_content);
                    
                    log_message(LOG_INFO, "Unlocking sentence %d for file %s", sentence_index, cmd.arg1);
                    release_write_lock(fl, sentence_index);
                }
            }
                } // End of acquire_write_lock else branch
            } // Close the get_or_create_file_lock block
        } else if (cmd.type == CMD_UNDO) {
            log_message(LOG_INFO, "Client requested UNDO for file: %s", cmd.arg1);
            char backup_name[MAX_FILENAME];
            snprintf(backup_name, sizeof(backup_name), ".%s.bak", cmd.arg1);
            
            FILE *backup_fp = fopen(backup_name, "r");
            if (!backup_fp) {
                send(client_socket, "ERROR: No undo history available\n", 33, 0);
            } else {
                FILE *fp = fopen(cmd.arg1, "w");
                if (fp) {
                    char temp_buf[4096];
                    size_t bytes_read;
                    while ((bytes_read = fread(temp_buf, 1, sizeof(temp_buf), backup_fp)) > 0) {
                        fwrite(temp_buf, 1, bytes_read, fp);
                    }
                    fclose(fp);
                    send(client_socket, "UNDO Successful!\n", 17, 0);
                } else {
                    send(client_socket, "ERROR: Failed to restore file\n", 30, 0);
                }
                fclose(backup_fp);
            }
        } else if (cmd.type == CMD_CREATE) {
            log_message(LOG_INFO, "Client requested CREATE for file: %s", cmd.arg1);
            FILE *fp = fopen(cmd.arg1, "w");
            if (fp) {
                fclose(fp);
                send(client_socket, "CREATE Successful!\n", 19, 0);
            } else {
                send(client_socket, "ERROR: Could not create file\n", 29, 0);
            }
        } else if (cmd.type == CMD_DELETE) {
            log_message(LOG_INFO, "Client requested DELETE for file: %s", cmd.arg1);
            if (remove(cmd.arg1) == 0) {
                send(client_socket, "DELETE Successful!\n", 19, 0);
            } else {
                send(client_socket, "ERROR: Could not delete file\n", 29, 0);
            }
        } else if (cmd.type == CMD_INFO) {
            log_message(LOG_INFO, "Client requested INFO for file: %s", cmd.arg1);
            struct stat st;
            if (stat(cmd.arg1, &st) == 0) {
                char info[MAX_BUFFER_SIZE];
                sprintf(info, "Size: %ld bytes\nPermissions: %o\n", st.st_size, st.st_mode & 0777);
                send(client_socket, info, strlen(info), 0);
            } else {
                send(client_socket, "ERROR: Could not retrieve file info\n", 36, 0);
            }
        } else if (cmd.type == CMD_STREAM) {
            log_message(LOG_INFO, "Client requested STREAM for file: %s", cmd.arg1);
            FILE *fp = fopen(cmd.arg1, "rb");
            if (fp) {
                char buffer[1024];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                    send(client_socket, buffer, bytes, 0);
                    // usleep(50000); // Simulate streaming delay if desired
                }
                fclose(fp);
                send(client_socket, "EOF", 3, 0);
            } else {
                send(client_socket, "ERROR: File not found\n", 22, 0);
            }
        } else if (cmd.type == CMD_EXEC) {
            log_message(LOG_INFO, "Client requested EXEC for file: %s", cmd.arg1);
            // Execute and stream output back
            char command[MAX_FILENAME + 2];
            snprintf(command, sizeof(command), "./%s", cmd.arg1);
            FILE *fp = popen(command, "r");
            if (fp) {
                char buffer[1024];
                while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                pclose(fp);
                send(client_socket, "EXECUTION COMPLETE\n", 19, 0);
            } else {
                send(client_socket, "ERROR: Could not execute file\n", 30, 0);
            }
        } else if (cmd.type == CMD_CREATE_DIR) {
            log_message(LOG_INFO, "Client requested MKDIR for directory: %s", cmd.arg1);
#ifdef _WIN32
            if (mkdir(cmd.arg1) == 0) {
#else
            if (mkdir(cmd.arg1, 0777) == 0) {
#endif
                send(client_socket, "MKDIR Successful!\n", 18, 0);
            } else {
                send(client_socket, "ERROR: Could not create directory\n", 34, 0);
            }
        } else if (cmd.type == CMD_LIST_DIR) {
            log_message(LOG_INFO, "Client requested LSDIR for directory: %s", cmd.arg1);
            char command[MAX_FILENAME + 16]; // Fix #5: enough room for 'ls -l ' prefix
#ifdef _WIN32
            snprintf(command, sizeof(command), "dir \"%s\"", cmd.arg1);
#else
            snprintf(command, sizeof(command), "ls -l \"%s\"", cmd.arg1);
#endif
            FILE *fp = popen(command, "r");
            if (fp) {
                char buffer[1024];
                while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                pclose(fp);
                send(client_socket, "EOF", 3, 0);
            } else {
                send(client_socket, "ERROR: Could not list directory\n", 32, 0);
            }
        } else if (cmd.type == CMD_REPLICATE) {
            log_message(LOG_INFO, "SS received REPLICATE for file: %s to %s", cmd.arg1, cmd.arg2);
            char arg2_copy[MAX_FILENAME]; // Fix #4: copy before strtok mutates struct
            strncpy(arg2_copy, cmd.arg2, sizeof(arg2_copy) - 1);
            arg2_copy[sizeof(arg2_copy) - 1] = '\0';
            char *ip = strtok(arg2_copy, ":");
            char *port_str = strtok(NULL, ":");
            if (ip && port_str) {
                int target_port = atoi(port_str);
                int repl_sock;
                struct sockaddr_in repl_addr;
                if ((repl_sock = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
                    repl_addr.sin_family = AF_INET;
                    repl_addr.sin_port = htons(target_port);
                    inet_pton(AF_INET, ip, &repl_addr.sin_addr);
                    if (connect(repl_sock, (struct sockaddr *)&repl_addr, sizeof(repl_addr)) == 0) {
                        ClientCommand repl_cmd;
                        memset(&repl_cmd, 0, sizeof(ClientCommand));
                        repl_cmd.type = CMD_CREATE;
                        strncpy(repl_cmd.arg1, cmd.arg1, MAX_FILENAME);
                        send(repl_sock, &repl_cmd, sizeof(repl_cmd), 0);
                        close(repl_sock);
                        log_message(LOG_INFO, "Successfully forwarded REPLICATE as CREATE to target SS");
                    }
                }
            }
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
