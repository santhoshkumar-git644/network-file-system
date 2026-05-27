#ifndef SS_FILE_MANAGER_H
#define SS_FILE_MANAGER_H

#include "common.h"
#include <pthread.h>

#define MAX_OPEN_FILES 50
#define MAX_SENTENCES_PER_FILE 100

typedef struct {
    char filename[MAX_FILENAME];
    pthread_rwlock_t sentence_locks[MAX_SENTENCES_PER_FILE];
    int is_used;
} SSFileLock;

void init_file_manager();
SSFileLock* get_or_create_file_lock(const char* filename);
int acquire_read_lock(SSFileLock* fl, int sentence_index, int wait);
int acquire_write_lock(SSFileLock* fl, int sentence_index, int wait);
void release_read_lock(SSFileLock* fl, int sentence_index);
void release_write_lock(SSFileLock* fl, int sentence_index);

#endif // SS_FILE_MANAGER_H
