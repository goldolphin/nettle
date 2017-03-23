/**
 * @author caofuxiang
 *         2015-05-11 16:51:51.
 */
#include <string.h>
#include <stdbool.h>
#include "logger.h"
#include "memory.h"

const char * PRIORITY_NAMES[] = {"ERROR", "WARNING", "INFO", "DEBUG", "ALL"};
log_priority_t g_log_priority = LOG_ALL;

void log_set_priority(log_priority_t priority) {
    g_log_priority = priority;
}

log_priority_t log_parse_priority(const char * str) {
    for (int i = 0; i < array_size(PRIORITY_NAMES); ++i) {
        if (strcmp(PRIORITY_NAMES[i], str) == 0) {
            return (log_priority_t) i;
        }
    }
    ensure(false);
}
