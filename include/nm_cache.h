#ifndef NM_CACHE_H
#define NM_CACHE_H

#include "common.h"
#include <pthread.h>

#define CACHE_SIZE 10

typedef struct CacheNode {
    char filename[MAX_FILENAME];
    int ss_id;
    struct CacheNode* prev;
    struct CacheNode* next;
} CacheNode;

typedef struct {
    CacheNode* head;
    CacheNode* tail;
    int count;
    pthread_mutex_t lock;
} LRUCache;

void cache_init();
int cache_get(const char* filename);
void cache_put(const char* filename, int ss_id);
void cache_invalidate(const char* filename);

#endif // NM_CACHE_H
