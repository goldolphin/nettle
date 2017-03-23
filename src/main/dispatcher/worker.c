/**
 * @author caofuxiang
 *         2015-06-30 11:36:36.
 */

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/memory.h>
#include "worker.h"

typedef struct {
    void * data;
    worker_free_t free;
} message_t;

static void callback (dispatcher_t * dispatcher, int fd, uint32_t event, void * data) {
    worker_t * worker = data;
    uint64_t num;
    ssize_t s = read(fd, &num, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) {
        return;
    }
    for (uint64_t i = 0; i < num; ++i) {
        message_t * msg;
        // Should not fail.
        ensure(conqueue_pop(&worker->mq, (void **) &msg));
        worker->on_message(dispatcher, worker, worker->extra, msg->data);
        msg->free(msg->data);
        pool_free(msg);
    }
}

error_t worker_init(worker_t * worker, size_t mq_capacity, void * extra, worker_on_message_t on_message) {
    // Init handler;
    int fd = eventfd(0, O_NONBLOCK);
    if (fd < 0) return errno;

    uint32_t events = EPOLLIN;
    handler_init(&worker->handler, fd, events, callback, worker);

    // Init message queue.
    conqueue_init(&worker->mq, mq_capacity);
    pool_init(&worker->pool, mq_capacity, sizeof(message_t));
    worker->mq_capacity = mq_capacity;

    // Init callback.
    worker->on_message = on_message;
    worker->extra = extra;
    return 0;
}

error_t worker_destroy(worker_t * worker) {
    // Destroy mq.
    while (true) {
        message_t * msg;
        if (!conqueue_pop(&worker->mq, (void **) &msg)) {
            break;
        }
        msg->free(msg->data);
        pool_free(msg);
    }
    conqueue_destroy(&worker->mq);

    // Destroy pool.
    pool_destroy(&worker->pool);

    // Close fd.
    if (close(worker->handler.fd) != 0) {
        return errno;
    }
    return 0;
}

bool worker_send(worker_t *worker, void *message, worker_free_t free) {
    message_t * m = pool_new(&worker->pool);
    if (m == NULL) {
        return false;
    }

    m->data = message;
    m->free = free;
    // Should not fail.
    ensure(conqueue_push(&worker->mq, m));

    uint64_t num = 1;
    ssize_t s = write(worker->handler.fd, &num, sizeof(uint64_t));
    // Should not fail.
    ensure(s == sizeof(uint64_t));
    return true;
}

error_t dispatcher_add_worker(dispatcher_t * dispatcher, worker_t * worker) {
    return dispatcher_add_handler(dispatcher, worker->handler.fd, &worker->handler);
}
