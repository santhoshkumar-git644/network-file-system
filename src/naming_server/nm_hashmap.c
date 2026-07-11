#include "nm_hashmap.h"
#include <pthread.h>

static HashNode* hashmap[HASHMAP_SIZE];
static pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % HASHMAP_SIZE;
}

void hashmap_init() {
    pthread_mutex_lock(&map_mutex);
    for (int i = 0; i < HASHMAP_SIZE; i++) {
        hashmap[i] = NULL;
    }
    pthread_mutex_unlock(&map_mutex);
}

int hashmap_insert(const char* filename, int ss_id) {
    unsigned int idx = hash(filename);
    pthread_mutex_lock(&map_mutex);
    
    HashNode* curr = hashmap[idx];
    while (curr) {
        if (strcmp(curr->filename, filename) == 0) {
            curr->ss_id = ss_id; // update
            pthread_mutex_unlock(&map_mutex);
            return 0;
        }
        curr = curr->next;
    }
    
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    strncpy(new_node->filename, filename, MAX_FILENAME - 1);
    new_node->filename[MAX_FILENAME - 1] = '\0'; // Fix #14: ensure null termination
    new_node->ss_id = ss_id;
    new_node->next = hashmap[idx];
    hashmap[idx] = new_node;
    
    pthread_mutex_unlock(&map_mutex);
    return 1;
}

int hashmap_lookup(const char* filename) {
    unsigned int idx = hash(filename);
    pthread_mutex_lock(&map_mutex);
    
    HashNode* curr = hashmap[idx];
    while (curr) {
        if (strcmp(curr->filename, filename) == 0) {
            int id = curr->ss_id;
            pthread_mutex_unlock(&map_mutex);
            return id;
        }
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&map_mutex);
    return -1; // Not found
}

void hashmap_delete(const char* filename) {
    unsigned int idx = hash(filename);
    pthread_mutex_lock(&map_mutex);
    
    HashNode* curr = hashmap[idx];
    HashNode* prev = NULL;
    
    while (curr) {
        if (strcmp(curr->filename, filename) == 0) {
            if (prev) {
                prev->next = curr->next;
            } else {
                hashmap[idx] = curr->next;
            }
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&map_mutex);
}

void hashmap_search(const char* substring, char* results, int max_len) {
    results[0] = '\0';
    int current_len = 0;
    
    pthread_mutex_lock(&map_mutex);
    for (int i = 0; i < HASHMAP_SIZE; i++) {
        HashNode* curr = hashmap[i];
        while (curr) {
            if (strstr(curr->filename, substring) != NULL) {
                int add_len = snprintf(results + current_len, max_len - current_len, "%s (SS: %d)\n", curr->filename, curr->ss_id);
                if (add_len > 0) {
                    current_len += add_len;
                }
                if (current_len >= max_len - 1) {
                    pthread_mutex_unlock(&map_mutex);
                    return;
                }
            }
            curr = curr->next;
        }
    }
    pthread_mutex_unlock(&map_mutex);
    
    if (current_len == 0) {
        snprintf(results, max_len, "No matching files/directories found.\n");
    }
}
