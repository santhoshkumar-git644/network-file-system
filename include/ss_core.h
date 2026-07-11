#ifndef SS_CORE_H
#define SS_CORE_H

#include "common.h"

// Define a structure to hold Storage Server details
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int nm_port;
    int client_port;
    char files[MAX_BUFFER_SIZE]; // simple comma separated list of files for now
} SS_Info;

void connect_to_nm(const char* nm_ip, int nm_port, int client_port);
void scan_local_directory(char* file_list, size_t max_len);

#endif // SS_CORE_H
