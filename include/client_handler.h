#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "common.h"

// Define command structures
typedef enum {
    CMD_VIEW,
    CMD_LIST_USERS,
    CMD_CREATE,
    CMD_DELETE,
    CMD_INFO,
    CMD_READ,
    CMD_WRITE,
    CMD_UNDO,
    CMD_STREAM,
    CMD_EXEC,
    CMD_UNKNOWN
} CommandType;

typedef struct {
    CommandType type;
    char arg1[MAX_BUFFER_SIZE];
    char arg2[MAX_BUFFER_SIZE];
} ClientCommand;

void* handle_client_connection(void* arg);

#endif // CLIENT_HANDLER_H
