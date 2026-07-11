#include "nm_users.h"
#include <string.h>
#include <pthread.h>

static User users[MAX_USERS];
static int num_users = 0;
static pthread_mutex_t users_lock = PTHREAD_MUTEX_INITIALIZER;

void init_users() {
    num_users = 0;
}

int add_user(const char* username) {
    pthread_mutex_lock(&users_lock);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&users_lock);
            return 0; // Already exists
        }
    }
    if (num_users < MAX_USERS) {
        strncpy(users[num_users].username, username, MAX_USERNAME - 1); // Fix #13
        users[num_users].username[MAX_USERNAME - 1] = '\0';
        users[num_users].num_files = 0;
        num_users++;
        pthread_mutex_unlock(&users_lock);
        return 1;
    }
    pthread_mutex_unlock(&users_lock);
    return -1;
}

int user_exists(const char* username) {
    pthread_mutex_lock(&users_lock);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&users_lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&users_lock);
    return 0;
}

int grant_access(const char* username, const char* filename) {
    pthread_mutex_lock(&users_lock);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (users[i].num_files < MAX_PERMISSIONS_PER_USER) {
                strncpy(users[i].accessible_files[users[i].num_files], filename, MAX_FILENAME);
                users[i].num_files++;
                pthread_mutex_unlock(&users_lock);
                return 1;
            }
            break;
        }
    }
    pthread_mutex_unlock(&users_lock);
    return 0;
}

int has_access(const char* username, const char* filename) {
    pthread_mutex_lock(&users_lock);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            for (int j = 0; j < users[i].num_files; j++) {
                if (strcmp(users[i].accessible_files[j], filename) == 0) {
                    pthread_mutex_unlock(&users_lock);
                    return 1;
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&users_lock);
    return 0;
}

void list_users(char* output, int max_len) {
    output[0] = '\0';
    int current_len = 0;
    pthread_mutex_lock(&users_lock);
    for (int i = 0; i < num_users; i++) {
        int add_len = snprintf(output + current_len, max_len - current_len,
                               "--> %s\n", users[i].username);
        if (add_len > 0) current_len += add_len;
        if (current_len >= max_len - 1) break;
    }
    if (num_users == 0) {
        snprintf(output, max_len, "No users registered.\n");
    }
    pthread_mutex_unlock(&users_lock);
}
