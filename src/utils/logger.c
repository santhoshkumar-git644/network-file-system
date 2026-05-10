#include "logger.h"
#include <stdarg.h>
#include <time.h>

static FILE *log_fp = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_logger(const char *log_file) {
    if (log_file != NULL) {
        log_fp = fopen(log_file, "a");
        if (!log_fp) {
            perror("Failed to open log file");
        }
    }
}

void log_message(LogLevel level, const char *format, ...) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    
    const char *level_str = "INFO";
    switch(level) {
        case LOG_INFO: level_str = "INFO"; break;
        case LOG_WARN: level_str = "WARN"; break;
        case LOG_ERROR: level_str = "ERROR"; break;
        case LOG_DEBUG: level_str = "DEBUG"; break;
    }
    
    FILE *out = (level == LOG_ERROR) ? stderr : stdout;
    
    fprintf(out, "[%s] [%s] ", time_str, level_str);
    if (log_fp) {
        fprintf(log_fp, "[%s] [%s] ", time_str, level_str);
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(out, format, args);
    va_end(args);
    
    if (log_fp) {
        va_start(args, format);
        vfprintf(log_fp, format, args);
        va_end(args);
        fprintf(log_fp, "\n");
        fflush(log_fp);
    }
    
    fprintf(out, "\n");
    
    pthread_mutex_unlock(&log_mutex);
}

void close_logger() {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}
