/**
 * @author caofuxiang
 *         2015-07-10 11:09:09.
 */

#ifndef NETTLE_COUNTER_H
#define NETTLE_COUNTER_H

#include <stdint.h>

typedef struct {
    volatile uint64_t value;
} counter_t;

static inline void counter_init(counter_t * counter) {
    counter->value = 0;
}

static inline void counter_inc(counter_t * counter, int n) {
    __sync_add_and_fetch(&counter->value, n);
}

static inline uint64_t counter_value(counter_t * counter) {
    return counter->value;
}

#endif //NETTLE_COUNTER_H
