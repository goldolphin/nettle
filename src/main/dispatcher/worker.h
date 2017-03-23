/**
 * @author caofuxiang
 *         2015-06-30 11:36:36.
 */

#ifndef NETTLE_WORKER_H
#define NETTLE_WORKER_H

#include "dispatcher.h"
#include "pool.h"
#include <utils/conqueue.h>

typedef void (* worker_free_t) (void * data);

struct worker_s;
typedef struct worker_s worker_t;

typedef void (* worker_on_message_t)(dispatcher_t * dispatcher, worker_t * worker, void * extra, void * message);

struct worker_s {
    handler_t handler;
    conqueue_t mq;
    pool_t pool;
    size_t mq_capacity;
    size_t mq_size;
    worker_on_message_t on_message;
    void * extra;
};

error_t worker_init(worker_t * worker, size_t mq_capacity, void * extra, worker_on_message_t on_message);

error_t worker_destroy(worker_t * worker);

bool worker_send(worker_t *worker, void *message, worker_free_t free);

static inline size_t worker_message_count(worker_t * worker) {
    return conqueue_size(&worker->mq);
}

error_t dispatcher_add_worker(dispatcher_t * dispatcher, worker_t * worker);

#endif //NETTLE_WORKER_H
