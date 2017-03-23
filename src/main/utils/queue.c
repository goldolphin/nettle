/**
 * @author caofuxiang
 *         2015-06-29 19:58:58.
 */

#include "memory.h"
#include "queue.h"

#define AT(p) queue->entries[p]
#define NEXT(p) (((p)+1)%queue->entry_num)
#define FORWARD(p) p = NEXT(p)

void queue_init(queue_t * queue, size_t capacity) {
    queue->capacity = capacity;
    queue->entry_num = capacity+1;
    queue->head = 0;
    queue->tail = 0;
    queue->entries = new_array(queue_entry_t, queue->entry_num);
    zero_array(queue->entries, queue->entry_num);
}

void queue_destroy(queue_t * queue) {
    free(queue->entries);
}

bool queue_push(queue_t * queue, void * value) {
    if (queue_size(queue) >= queue->capacity) {
        return false;
    }
    AT(queue->tail).value = value;
    FORWARD(queue->tail);
    return true;
}

bool queue_pop(queue_t * queue, void ** value) {
    if (queue_size(queue) == 0) {
        return false;
    }
    *value = AT(queue->head).value;
    FORWARD(queue->head);
    return true;
}

/**
 * Return the next valid iterator, or queue_end(queue) if there're no more bindings after current iterator.
 */
queue_iterator_t queue_next(queue_t * queue, queue_iterator_t iter, void ** value) {
    ensure(iter != queue_end(queue));
    *value = AT(queue->head).value;
    return NEXT(iter);
}
