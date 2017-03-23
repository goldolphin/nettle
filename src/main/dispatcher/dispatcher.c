/**
 * @author caofuxiang
 *         2015-05-06 16:15:15.
 */

#include <unistd.h>
#include <utils/utils.h>
#include "dispatcher.h"

void handler_init(handler_t * handler, int fd, uint32_t events, callback_t callback, void * data) {
    handler->fd = fd;
    handler->events = events;
    handler->callback = callback;
    handler->data = data;
}

typedef struct {
    int delay;
    bool is_permanent;
    timer_callback_t callback;
    void * data;
} timer_entry_t;

timer_entry_t * make_timer(int delay, bool is_permanent, timer_callback_t callback, void *data) {
    timer_entry_t * timer = new_data(timer_entry_t);
    timer->delay = delay;
    timer->is_permanent = is_permanent;
    timer->callback = callback;
    timer->data = data;
    return timer;
}

void destroy_timer(timer_entry_t * timer) {
    free(timer);
}

error_t dispatcher_init(dispatcher_t * dispatcher, int max_events_per_wait, int default_wait_timeout, size_t timer_queue_capacity, void (* pre_loop) (dispatcher_t *)) {
    dispatcher->epollfd = epoll_create1(0);
    if (dispatcher->epollfd < 0) {
        return errno;
    }

    dispatcher->max_events_per_wait = max_events_per_wait;
    dispatcher->default_wait_timeout = default_wait_timeout;
    dispatcher->pre_loop = pre_loop;

    dispatcher->timer_queue = make_heap(timer_queue_capacity);
    dispatcher->to_stop = false;
    dispatcher->error = 0;
    return 0;
}

error_t dispatcher_init1(dispatcher_t * dispatcher, int default_wait_timeout) {
    return dispatcher_init(dispatcher, 10, default_wait_timeout, 1000, NULL);
}

error_t dispatcher_destroy(dispatcher_t * dispatcher) {
    if (dispatcher->epollfd >= 0) {
        if (close(dispatcher->epollfd) != 0) {
            return errno;
        }

        while (true) {
            long deadline;
            timer_entry_t * timer;
            if (!heap_pop(dispatcher->timer_queue, &deadline, (void **)&timer)) {
                break;
            }
            destroy_timer(timer);
        }
        destroy_heap(dispatcher->timer_queue);

        dispatcher->epollfd = -1;
        dispatcher->to_stop = false;
        dispatcher->error = 0;
    }
    return 0;
}

error_t dispatcher_add_handler(dispatcher_t * dispatcher, int fd, handler_t * handler) {
    struct epoll_event event;
    event.events = handler->events;
    event.data.ptr = handler;
    if (epoll_ctl(dispatcher->epollfd, EPOLL_CTL_ADD, fd, &event) != 0) {
        return errno;
    } else {
        return 0;
    }
}

error_t dispatcher_remove_handler(dispatcher_t * dispatcher, int fd) {
    struct epoll_event event;
    if (epoll_ctl(dispatcher->epollfd, EPOLL_CTL_DEL, fd, &event) != 0) {
        return errno;
    } else {
        return 0;
    }
}

static inline bool dispatcher_add_timer0(dispatcher_t * dispatcher, timer_entry_t * timer) {
    long deadline = current_millis() + timer->delay;
    return heap_push(dispatcher->timer_queue, deadline, timer);
}

error_t dispatcher_add_timer(dispatcher_t * dispatcher, int delay, bool is_permanent, timer_callback_t callback, void *data) {
    timer_entry_t * timer = make_timer(delay, is_permanent, callback, data);
    if (dispatcher_add_timer0(dispatcher, timer)) {
        return 0;
    }
    destroy_timer(timer);
    return ENOMEM;
}

void dispatcher_stop(dispatcher_t * dispatcher, error_t error) {
    dispatcher->to_stop = true;
    dispatcher->error = error;
}

error_t dispatcher_run(dispatcher_t * dispatcher) {
    dispatcher->to_stop = false;
    dispatcher->error = 0;

    struct epoll_event * evs = new_array(struct epoll_event, dispatcher->max_events_per_wait);
    while (true) {
        if (dispatcher->pre_loop != NULL) {
            dispatcher->pre_loop(dispatcher);
        }

        if (dispatcher->to_stop) {
            error_t error = dispatcher->error;
            dispatcher->to_stop = false;
            dispatcher->error = 0;
            free(evs);
            return error;
        }

        // Process timers.
        int wait_timeout = dispatcher->default_wait_timeout;
        while (true) {
            long deadline;
            timer_entry_t * timer;
            if (!heap_head(dispatcher->timer_queue, &deadline, (void **)&timer)) {
                break;
            }
            long interval = deadline - current_millis();
            if (interval > 0) {
                wait_timeout = (int)interval;
                break;
            } else {
                heap_pop(dispatcher->timer_queue, NULL, NULL);
                timer->callback(dispatcher, timer->data);
                if (timer->is_permanent) {
                    ensure(dispatcher_add_timer0(dispatcher, timer)); // Must succeed!
                } else {
                    destroy_timer(timer);
                }
            }
        }

        // Process fds.
        int nfds = epoll_wait(dispatcher->epollfd, evs, dispatcher->max_events_per_wait, wait_timeout);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            free(evs);
            return errno;
        }

        for (int i = 0; i < nfds; ++i) {
            handler_t * handler = (handler_t *)(evs[i].data.ptr);
            handler->callback(dispatcher, handler->fd, evs[i].events, handler->data);
        }
    }
}
