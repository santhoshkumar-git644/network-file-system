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
        strncpy(users[num_users].username, username, MAX_FILENAME);
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
