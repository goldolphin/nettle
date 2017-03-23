/**
 * @author caofuxiang
 *         2015-06-29 19:58:58.
 */

#ifndef NETTLE_QUEUE_H
#define NETTLE_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void * value;
} queue_entry_t;

typedef struct {
    size_t capacity;
    size_t entry_num;
    size_t head;
    size_t tail;
    queue_entry_t * entries;
} queue_t;

typedef size_t queue_iterator_t;

void queue_init(queue_t * queue, size_t capacity);

void queue_destroy(queue_t * queue);

static inline size_t queue_size(queue_t * queue) {
    return (queue->entry_num + queue->tail - queue->head) % queue->entry_num;
}

bool queue_push(queue_t * queue, void * value);

bool queue_pop(queue_t * queue, void ** value);

static inline queue_iterator_t queue_begin(queue_t * queue) {
    return queue->head;
}

static inline queue_iterator_t queue_end(queue_t * queue) {
    return queue->tail;
}

queue_iterator_t queue_next(queue_t * queue, queue_iterator_t iter, void ** value);

#endif //NETTLE_QUEUE_H
