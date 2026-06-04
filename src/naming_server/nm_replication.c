#include "nm_replication.h"
#include "nm_core.h"
#include "client_handler.h"
#include "logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

void trigger_replication(const char* filename, int source_ss_id) {
    if (source_ss_id < 0 || source_ss_id >= MAX_STORAGE_SERVERS || !ss_list[source_ss_id].is_active) {
        return;
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
        return;
    }
    
    // Connect to source SS and tell it to replicate to target SS
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ss_list[source_ss_id].info.client_port);
    inet_pton(AF_INET, ss_list[source_ss_id].info.ip, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return;
    }
    
    ClientCommand cmd;
    memset(&cmd, 0, sizeof(ClientCommand));
    cmd.type = CMD_REPLICATE;
    strncpy(cmd.arg1, filename, MAX_FILENAME);
    // arg2 can hold target SS IP:PORT
    snprintf(cmd.arg2, sizeof(cmd.arg2), "%s:%d", ss_list[target_ss_id].info.ip, ss_list[target_ss_id].info.client_port);
    
    send(sock, &cmd, sizeof(cmd), 0);
    close(sock);
    
    log_message(LOG_INFO, "Triggered replication of %s from SS %d to SS %d", filename, source_ss_id, target_ss_id);
}
