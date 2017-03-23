/**
 * @author caofuxiang
 *         2015-06-30 09:55:55.
 */

#ifndef NETTLE_CONQUEUE_H
#define NETTLE_CONQUEUE_H

#include <pthread.h>
#include "queue.h"

/**
 * A concurrent queue.
 */
typedef struct {
    queue_t queue;
    pthread_mutex_t lock;
} conqueue_t;

void conqueue_init(conqueue_t * queue, size_t capacity);

void conqueue_destroy(conqueue_t * queue);

size_t conqueue_size(conqueue_t * queue);

bool conqueue_push(conqueue_t * queue, void * value);

bool conqueue_pop(conqueue_t * queue, void ** value);

#endif //NETTLE_CONQUEUE_H
