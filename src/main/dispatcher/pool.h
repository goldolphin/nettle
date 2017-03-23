/**
 * @author caofuxiang
 *         2015-07-02 10:33:33.
 */

#ifndef NETTLE_POOL_H
#define NETTLE_POOL_H

#include <stddef.h>
#include <utils/conqueue.h>

typedef struct {
    conqueue_t queue;
    size_t capacity;
    size_t size;
} pool_t;

void pool_init(pool_t * pool, size_t capacity, size_t size);

void pool_destroy(pool_t * pool);

/**
 * Retured pointer (if not null) must be freed by calling pool_free().
 */
void * pool_new(pool_t * pool);

/**
 * buffer must be the returned value of pool_new().
 */
void pool_free(void * buffer);

#endif //NETTLE_POOL_H
