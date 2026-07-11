#include "nm_replication.h"
#include "nm_core.h"
#include "client_handler.h"
#include "logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Struct to pass args to the replication thread
typedef struct {
    char filename[MAX_FILENAME];
    int source_ss_id;
} ReplicationArgs;

static void* replication_thread(void* arg) {
    ReplicationArgs* rargs = (ReplicationArgs*)arg;
    const char* filename = rargs->filename;
    int source_ss_id = rargs->source_ss_id;
    
    if (source_ss_id < 0 || source_ss_id >= MAX_STORAGE_SERVERS || !ss_list[source_ss_id].is_active) {
        free(rargs);
        return NULL;
    }
    
    // Find target SS
    int target_ss_id = -1;
    for (int i = 0; i < MAX_STORAGE_SERVERS; i++) {
        if (i != source_ss_id && ss_list[i].is_active) {
            target_ss_id = i;
            break;
        }
    }
    
    if (target_ss_id == -1) {
        log_message(LOG_WARN, "No target SS available for replication of %s", filename);
        free(rargs);
        return NULL;
    }
    
    // Connect to source SS and tell it to replicate to target SS
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        free(rargs);
        return NULL;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ss_list[source_ss_id].info.client_port);
    inet_pton(AF_INET, ss_list[source_ss_id].info.ip, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        free(rargs);
        return NULL;
    }
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_REPLICATE;
    strncpy(cmd.arg1, filename, MAX_FILENAME - 1);
    // arg2 can hold target SS IP:PORT
    snprintf(cmd.arg2, sizeof(cmd.arg2), "%s:%d", ss_list[target_ss_id].info.ip, ss_list[target_ss_id].info.client_port);
    
    send(sock, &cmd, sizeof(cmd), 0);
    close(sock);
    
    log_message(LOG_INFO, "Triggered replication of %s from SS %d to SS %d", filename, source_ss_id, target_ss_id);
    free(rargs);
    return NULL;
}

// Fix #12: trigger_replication now spawns a detached thread so NM is not blocked
void trigger_replication(const char* filename, int source_ss_id) {
    ReplicationArgs* args = malloc(sizeof(ReplicationArgs));
    strncpy(args->filename, filename, MAX_FILENAME - 1);
    args->filename[MAX_FILENAME - 1] = '\0';
    args->source_ss_id = source_ss_id;
    
    pthread_t t;
    if (pthread_create(&t, NULL, replication_thread, args) == 0) {
        pthread_detach(t);
    } else {
        free(args); // thread failed to start, free args
    }
}
