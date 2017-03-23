/**
 * @author caofuxiang
 *         2015-07-02 10:33:33.
 */

#include <stdint.h>
#include <utils/memory.h>
#include "pool.h"

struct pool_buffer_s;
typedef struct pool_buffer_s pool_buffer_t;

struct pool_buffer_s {
    pool_t * pool;
    uint8_t buffer[0];
};

void pool_init(pool_t * pool, size_t capacity, size_t size) {
    conqueue_init(&pool->queue, capacity);
    for (int i = 0; i < capacity; ++i) {
        pool_buffer_t *pb = (pool_buffer_t *) new_array(uint8_t, size + sizeof(pool_buffer_t));
        pb->pool = pool;
        conqueue_push(&pool->queue, pb);
    }

    pool->capacity = capacity;
    pool->size = size;
}

void pool_destroy(pool_t * pool) {
    while (true) {
        pool_buffer_t *pb;
        if (!conqueue_pop(&pool->queue, (void **) &pb)) {
            break;
        }
        free(pb);
    }
    conqueue_destroy(&pool->queue);
}

void * pool_new(pool_t * pool) {
    pool_buffer_t *pb;
    if (conqueue_pop(&pool->queue, (void **) &pb)) {
        return pb->buffer;
    }
    return NULL;
}

void pool_free(void * buffer) {
    pool_buffer_t *pb = container_of(buffer, pool_buffer_t, buffer);
    ensure(conqueue_push(&pb->pool->queue, pb));
}
