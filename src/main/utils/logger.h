/**
 * @author caofuxiang
 *         2015-05-07 13:47:47.
 */

#ifndef NETTLE_LOGGER_H
#define NETTLE_LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

typedef enum {
    LOG_ERR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_ALL,
} log_priority_t;

extern const char * PRIORITY_NAMES[];

extern log_priority_t g_log_priority;

void log_set_priority(log_priority_t priority);

log_priority_t log_parse_priority(const char * str);

static inline void vlog(log_priority_t priority, const char *message, va_list args) {
    if (priority < g_log_priority) {
        char buffer[4096];
        int n = vsnprintf(buffer, sizeof buffer, message, args);
        if (n >= 0) {
            struct tm tmv;
            time_t tv = time(NULL);
            localtime_r(&tv, &tmv);
            fprintf(stderr, "%4d-%02d-%02d %02d:%02d:%02d %s| %s\n",
                    tmv.tm_year+1900, tmv.tm_mon+1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
                    PRIORITY_NAMES[priority], buffer);
        }
    }
}

static inline void log_log(log_priority_t priority, const char *message, ...) {
    va_list args;
    va_start(args, message);
    vlog(priority, message, args);
    va_end(args);
}

static inline void log_debug(const char *message, ...) {
    va_list args;
    va_start(args, message);
    vlog(LOG_DEBUG, message, args);
    va_end(args);
}

static inline void log_info(const char *message, ...) {
    va_list args;
    va_start(args, message);
    vlog(LOG_INFO, message, args);
    va_end(args);
}

static inline void log_warn(const char *message, ...) {
    va_list args;
    va_start(args, message);
    vlog(LOG_WARNING, message, args);
    va_end(args);
}

static inline void log_error(const char *message, ...) {
    va_list args;
    va_start(args, message);
    vlog(LOG_ERR, message, args);
    va_end(args);
}

#endif //NETTLE_LOGGER_H
