#ifndef NM_CORE_H
#define NM_CORE_H

#include "common.h"
#include "ss_core.h" // For SS_Info

#define MAX_STORAGE_SERVERS 10

typedef struct {
    SS_Info info;
    int is_active;
    int socket_fd;
} StorageServerEntry;

void init_nm_state();
void start_nm_server();
void* handle_ss_connection(void* arg);

#endif // NM_CORE_H
