#include "ss_core.h"
#include "logger.h"
#include <dirent.h>

void scan_local_directory(char* file_list, size_t max_len) {
    DIR *dir;
    struct dirent *ent;
    file_list[0] = '\0'; // Initialize empty
    
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            // Only add regular files, skip . and ..
            if (ent->d_type == DT_REG) {
                size_t current_len = strlen(file_list);
                size_t entry_len = strlen(ent->d_name);
                if (current_len + entry_len + 2 < max_len) { // +2 for ',' and '\0'
                    if (current_len > 0) {
                        strcat(file_list, ",");
                    }
                    strcat(file_list, ent->d_name);
                }
            }
        }
        closedir(dir);
    } else {
        log_message(LOG_ERROR, "Could not open current directory for scanning");
    }
}

void connect_to_nm(const char* nm_ip, int nm_port, int client_port, const char* own_ip) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_message(LOG_ERROR, "Socket creation error");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(nm_port);
    
    if (inet_pton(AF_INET, nm_ip, &serv_addr.sin_addr) <= 0) {
        log_message(LOG_ERROR, "Invalid address/ Address not supported");
        return;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_message(LOG_ERROR, "Connection to NM Failed");
        return;
    }
    
    SS_Info info;
    strncpy(info.ip, own_ip, INET_ADDRSTRLEN - 1);
    info.ip[INET_ADDRSTRLEN - 1] = '\0';
    info.nm_port = nm_port;
    info.client_port = client_port;
    scan_local_directory(info.files, sizeof(info.files));
    
    log_message(LOG_INFO, "Sending SS Init packet. Files: %s", info.files);
    send(sock, &info, sizeof(info), 0);
    
    // We intentionally do NOT close(sock) here!
    // Keeping the socket open acts as a heartbeat for the NM to know this SS is alive.
}

void ss_init(int client_port) {
    log_message(LOG_INFO, "Initializing Storage Server on client port %d", client_port);
    // Connect to NM on default port 8080
    connect_to_nm("127.0.0.1", NM_PORT, client_port);
}
