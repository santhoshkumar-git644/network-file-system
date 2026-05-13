#ifndef NM_HASHMAP_H
#define NM_HASHMAP_H

#include "common.h"

#define HASHMAP_SIZE 1024

typedef struct HashNode {
    char filename[MAX_FILENAME];
    int ss_id; // Index in the ss_list
    struct HashNode* next;
} HashNode;

void hashmap_init();
int hashmap_insert(const char* filename, int ss_id);
int hashmap_lookup(const char* filename);
void hashmap_delete(const char* filename);

#endif // NM_HASHMAP_H
