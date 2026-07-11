#ifndef NM_USERS_H
#define NM_USERS_H

#include "common.h"

#define MAX_USERS 50
#define MAX_PERMISSIONS_PER_USER 100

typedef struct {
    char username[MAX_USERNAME];  // Fix #13: use MAX_USERNAME, not MAX_FILENAME
    char accessible_files[MAX_PERMISSIONS_PER_USER][MAX_FILENAME];
    int num_files;
} User;

void init_users();
int add_user(const char* username);
int user_exists(const char* username);
int grant_access(const char* username, const char* filename);
int has_access(const char* username, const char* filename);
void list_users(char* output, int max_len);

#endif // NM_USERS_H
