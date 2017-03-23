/**
 * @author caofuxiang
 *         2015-06-01 20:36:36.
 */

#include "sbuilder.h"
void sbuilder_init(sbuilder_t * builder, char * buf, size_t capacity) {
    builder->buf = buf;
    builder->capacity = capacity;
    builder->len = 0;
}

bool sbuilder_format(sbuilder_t * builder, const char *message, ...) {
    size_t left = builder->capacity - builder->len;
    if (left <= 1) {
        return false;
    }
    va_list args;
    va_start(args, message);
    int count = vsnprintf(builder->buf+builder->len, left, message, args);
    va_end(args);
    if (count >= left) {
        builder->len = builder->capacity;
    } else {
        builder->len += count;
    }
    return true;
}
