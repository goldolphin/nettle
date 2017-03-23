/**
 * @author caofuxiang
 *         2015-06-30 09:55:55.
 */

#include "conqueue.h"
#include "memory.h"

#define GUARD(stmt) do {\
ensure(pthread_mutex_lock(&queue->lock) == 0); \
{stmt} \
ensure(pthread_mutex_unlock(&queue->lock) == 0); \
} while(0)

void conqueue_init(conqueue_t * queue, size_t capacity) {
    ensure(pthread_mutex_init(&queue->lock, NULL) == 0);
    queue_init(&queue->queue, capacity);
}

void conqueue_destroy(conqueue_t * queue) {
    queue_destroy(&queue->queue);
    ensure(pthread_mutex_destroy(&queue->lock) == 0);
}

size_t conqueue_size(conqueue_t * queue) {
    size_t res;
    GUARD(res = queue_size(&queue->queue););
    return res;
}

bool conqueue_push(conqueue_t * queue, void * value) {
    bool res;
    GUARD(res = queue_push(&queue->queue, value););
    return res;
}

bool conqueue_pop(conqueue_t * queue, void ** value) {
    bool res;
    GUARD(res = queue_pop(&queue->queue, value););
    return res;
}
