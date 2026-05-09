#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 4096
#define MAX_FILENAME 256
#define MAX_USERNAME 64
#define MAX_PATH 1024

// Default ports if not specified
#define NM_PORT 8080

// Error Codes
#define ERR_NONE 0
#define ERR_FILE_NOT_FOUND 1
#define ERR_UNAUTHORIZED 2
#define ERR_FILE_LOCKED 3
#define ERR_INVALID_ARGS 4
#define ERR_SERVER_DOWN 5

#endif // COMMON_H
