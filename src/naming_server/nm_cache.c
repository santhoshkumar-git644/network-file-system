#include "nm_cache.h"
#include <stdlib.h>
#include <string.h>

static LRUCache cache;

void cache_init() {
    cache.head = NULL;
    cache.tail = NULL;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
}

static void move_to_front(CacheNode* node) {
    if (cache.head == node) return;
    
    // Remove from current position
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (cache.tail == node) cache.tail = node->prev;
    
    // Add to front
    node->next = cache.head;
    node->prev = NULL;
    if (cache.head) cache.head->prev = node;
    cache.head = node;
    if (!cache.tail) cache.tail = node;
}

int cache_get(const char* filename) {
    pthread_mutex_lock(&cache.lock);
    CacheNode* curr = cache.head;
    while (curr) {
        if (strcmp(curr->filename, filename) == 0) {
            move_to_front(curr);
            int ss_id = curr->ss_id;
            pthread_mutex_unlock(&cache.lock);
            return ss_id;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&cache.lock);
    return -1;
}

void cache_put(const char* filename, int ss_id) {
    pthread_mutex_lock(&cache.lock);
    
    // Check if exists
    CacheNode* curr = cache.head;
    while (curr) {
        if (strcmp(curr->filename, filename) == 0) {
            curr->ss_id = ss_id;
            move_to_front(curr);
            pthread_mutex_unlock(&cache.lock);
            return;
        }
        curr = curr->next;
    }
    
    // Add new
    CacheNode* new_node = (CacheNode*)malloc(sizeof(CacheNode));
    strncpy(new_node->filename, filename, MAX_FILENAME);
    new_node->ss_id = ss_id;
    new_node->prev = NULL;
    new_node->next = cache.head;
    
    if (cache.head) cache.head->prev = new_node;
    cache.head = new_node;
    if (!cache.tail) cache.tail = new_node;
    cache.count++;
    
    // Evict if over capacity
    if (cache.count > CACHE_SIZE) {
        CacheNode* temp = cache.tail;
        cache.tail = temp->prev;
        if (cache.tail) cache.tail->next = NULL;
        free(temp);
        cache.count--;
    }
    
    pthread_mutex_unlock(&cache.lock);
}

void cache_invalidate(const char* filename) {
    pthread_mutex_lock(&cache.lock);
    CacheNode* curr = cache.head;
    while (curr) {
        if (strcmp(curr->filename, filename) == 0) {
            if (curr->prev) curr->prev->next = curr->next;
            else cache.head = curr->next;
            
            if (curr->next) curr->next->prev = curr->prev;
            else cache.tail = curr->prev;
            
            free(curr);
            cache.count--;
            break;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&cache.lock);
}
