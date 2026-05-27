#include "ss_file_manager.h"
#include <string.h>
#include <errno.h>

SSFileLock open_files[MAX_OPEN_FILES];
pthread_mutex_t fm_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_file_manager() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].is_used = 0;
    }
}

SSFileLock* get_or_create_file_lock(const char* filename) {
    pthread_mutex_lock(&fm_mutex);
    
    // Find existing
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].is_used && strcmp(open_files[i].filename, filename) == 0) {
            pthread_mutex_unlock(&fm_mutex);
            return &open_files[i];
        }
    }
    
    // Create new
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].is_used) {
            strncpy(open_files[i].filename, filename, MAX_FILENAME);
            for (int j = 0; j < MAX_SENTENCES_PER_FILE; j++) {
                pthread_rwlock_init(&open_files[i].sentence_locks[j], NULL);
            }
            open_files[i].is_used = 1;
            pthread_mutex_unlock(&fm_mutex);
            return &open_files[i];
        }
    }
    
    pthread_mutex_unlock(&fm_mutex);
    return NULL; // Out of space
}

int acquire_read_lock(SSFileLock* fl, int sentence_index, int wait) {
    if (sentence_index < 0 || sentence_index >= MAX_SENTENCES_PER_FILE) return -1;
    if (wait) {
        return pthread_rwlock_rdlock(&fl->sentence_locks[sentence_index]);
    } else {
        return pthread_rwlock_tryrdlock(&fl->sentence_locks[sentence_index]) == 0 ? 0 : -1;
    }
}

int acquire_write_lock(SSFileLock* fl, int sentence_index, int wait) {
    if (sentence_index < 0 || sentence_index >= MAX_SENTENCES_PER_FILE) return -1;
    if (wait) {
        return pthread_rwlock_wrlock(&fl->sentence_locks[sentence_index]);
    } else {
        return pthread_rwlock_trywrlock(&fl->sentence_locks[sentence_index]) == 0 ? 0 : -1;
    }
}

void release_read_lock(SSFileLock* fl, int sentence_index) {
    if (sentence_index >= 0 && sentence_index < MAX_SENTENCES_PER_FILE) {
        pthread_rwlock_unlock(&fl->sentence_locks[sentence_index]);
    }
}

void release_write_lock(SSFileLock* fl, int sentence_index) {
    if (sentence_index >= 0 && sentence_index < MAX_SENTENCES_PER_FILE) {
        pthread_rwlock_unlock(&fl->sentence_locks[sentence_index]);
    }
}
