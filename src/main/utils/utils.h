/**
 * @author caofuxiang
 *         2015-07-08 17:08:08.
 */

#ifndef NETTLE_UTILS_H
#define NETTLE_UTILS_H

#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "memory.h"

#ifdef __MACH__
#include <sys/time.h>
#endif

/**
 * Return the current time in millis-seconds since some UNSPECIFIED starting point
 */
static inline long current_millis() {
#ifdef __MACH__
    struct timeval tv;
    gettimeofday(&tv, NULL); // Should not fail.
    return tv.tv_sec * 1000L + tv.tv_usec/1000L;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // Should not fail.
    return ts.tv_sec * 1000L + ts.tv_nsec/1000000L;
#endif
}

/**
 * Return the current clock time in millis-seconds.
 */
static inline long current_millis_real() {
#ifdef __MACH__
    struct timeval tv;
    gettimeofday(&tv, NULL); // Should not fail.
    return tv.tv_sec * 1000L + tv.tv_usec/1000L;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // Should not fail.
    return ts.tv_sec * 1000L + ts.tv_nsec/1000000L;
#endif
}

static inline bool str2bool(const char * str) {
    if (strcmp("true", str) == 0) {
        return true;
    } else if (strcmp("false", str) == 0) {
        return false;
    }
    ensure(false);
}

static inline bool contains(char c, const char * str) {
    while (true) {
        char s = str[0];
        if (s == '\0') return false;
        if (s == c) return true;
        str ++;
    }
}

static inline bool starts_with(const char *prefix, const char *str) {
    while (true) {
        char p = prefix[0];
        char s = str[0];
        if (p == '\0') {
            return true;
        }
        if (p != s) {
            return false;
        }
        ++ prefix;
        ++ str;
    }
}

static inline const char * skip(const char * set, const char * str) {
    while (true) {
        if (!contains(str[0], set)) {
            return str;
        }
        str ++;
    }
}

static inline const char *skip_delimit(const char *delimiter, const char *str) {
    str = skip(" \t", str);
    if (!starts_with(delimiter, str)) {
        return NULL;
    }
    return skip(" \t", str+strlen(delimiter));
}

typedef struct {
    volatile uint32_t id;
} idgen_t;

static inline void idgen_init(idgen_t * idgen, uint32_t initial) {
    idgen->id = initial;
}

static inline uint32_t idgen_next(idgen_t * idgen) {
    return __sync_fetch_and_add(&idgen->id, 1);
}

#endif //NETTLE_UTILS_H
