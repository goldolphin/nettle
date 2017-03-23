/**
 * @author caofuxiang
 *         2015-05-06 16:15:15.
 */

#ifndef NETTLE_DISPATCHER_H
#define NETTLE_DISPATCHER_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <utils/heap.h>
#include "common.h"

typedef struct dispatcher_s {
    int epollfd;
    int max_events_per_wait;
    int default_wait_timeout;
    void (* pre_loop) (struct dispatcher_s *);

    heap_t * timer_queue;
    bool to_stop;
    error_t error;
} dispatcher_t;

typedef void (* callback_t) (dispatcher_t * dispatcher, int fd, uint32_t event, void * data);

typedef void (* timer_callback_t) (dispatcher_t * dispatcher, void * data);

typedef struct {
    int fd;
    uint32_t events;
    callback_t callback;
    void * data;
} handler_t;

void handler_init(handler_t * handler, int fd, uint32_t events, callback_t callback, void * data);

error_t dispatcher_init(dispatcher_t * dispatcher, int max_events_per_wait, int default_wait_timeout /* in ms */,
                        size_t timer_queue_capacity, void (* pre_loop) (dispatcher_t *));

error_t dispatcher_init1(dispatcher_t * dispatcher, /* int max_events_per_wait = 10, */ int wait_timeout /* in ms */
                         /* size_t timer_queue_capacity = 1000 */);

error_t dispatcher_destroy(dispatcher_t * dispatcher);

error_t dispatcher_add_handler(dispatcher_t * dispatcher, int fd, handler_t * handler);

error_t dispatcher_remove_handler(dispatcher_t * dispatcher, int fd);

error_t dispatcher_add_timer(dispatcher_t * dispatcher, int delay /* in ms */, bool is_permanent, timer_callback_t callback, void *data);

void dispatcher_stop(dispatcher_t * dispatcher, error_t error);

error_t dispatcher_run(dispatcher_t * dispatcher);

#endif //NETTLE_DISPATCHER_H
