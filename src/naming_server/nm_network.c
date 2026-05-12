#include "nm_core.h"
#include "logger.h"

StorageServerEntry ss_list[MAX_STORAGE_SERVERS];
int ss_count = 0;
pthread_mutex_t ss_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_nm_state() {
    for (int i = 0; i < MAX_STORAGE_SERVERS; i++) {
        ss_list[i].is_active = 0;
    }
}

void* handle_ss_connection(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    SS_Info info;
    int bytes_read = recv(client_socket, &info, sizeof(info), 0);
    if (bytes_read > 0) {
        pthread_mutex_lock(&ss_list_mutex);
        if (ss_count < MAX_STORAGE_SERVERS) {
            ss_list[ss_count].info = info;
            ss_list[ss_count].is_active = 1;
            ss_list[ss_count].socket_fd = client_socket;
            log_message(LOG_INFO, "SS Registered: IP %s, NM Port %d, Client Port %d", 
                        info.ip, info.nm_port, info.client_port);
            log_message(LOG_INFO, "SS Files: %s", info.files);
            ss_count++;
        } else {
            log_message(LOG_ERROR, "Max Storage Servers reached. Connection rejected.");
            close(client_socket);
        }
        pthread_mutex_unlock(&ss_list_mutex);
    } else {
        log_message(LOG_ERROR, "Failed to receive SS Init packet");
        close(client_socket);
    }
    
    // In a real implementation, we would keep this thread alive to monitor the SS
    // For now, we just register it.
    
    return NULL;
}
