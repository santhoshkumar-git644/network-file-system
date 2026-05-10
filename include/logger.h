#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

void init_logger(const char *log_file);
void log_message(LogLevel level, const char *format, ...);
void close_logger();

#endif // LOGGER_H
