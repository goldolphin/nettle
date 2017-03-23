/**
 * @author caofuxiang
 *         2015-07-23 10:37:37.
 */

#ifndef NETTLE_LIST_C_H
#define NETTLE_LIST_C_H

#include <stddef.h>
#include "memory.h"

/**
 * A intrusive doubly linked list.
 */
struct list_entry_s;
typedef struct list_entry_s list_entry_t;

struct list_entry_s {
    list_entry_t * prev;
    list_entry_t * next;
};

typedef struct {
    list_entry_t head;
    list_entry_t tail;
} list_t;

void list_init(list_t * list);

static inline void list_push_front(list_t * list, list_entry_t * entry) {
    entry->prev = &list->head;
    entry->next = list->head.next;
    list->head.next = entry;
    entry->next->prev = entry;
}

static inline void list_push_back(list_t * list, list_entry_t * entry) {
    entry->prev = list->tail.prev;
    entry->next = &list->tail;
    list->tail.prev = entry;
    entry->prev->next = entry;
}

static inline void list_remove(list_entry_t * entry) {
    ensure(entry->next != NULL && entry->prev != NULL);
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->prev = NULL;
    entry->next = NULL;
}

static inline list_entry_t * list_pop_front(list_t * list) {
    list_entry_t * entry = list->head.next;
    list_remove(entry);
    return entry;
}

static inline list_entry_t * list_pop_back(list_t * list) {
    list_entry_t * entry = list->tail.prev;
    list_remove(entry);
    return entry;
}

static inline list_entry_t * list_peek_front(list_t * list) {
    list_entry_t * entry = list->head.next;
    ensure(entry->next != NULL && entry->prev != NULL);
    return entry;
}

static inline list_entry_t * list_peek_back(list_t * list) {
    list_entry_t * entry = list->tail.prev;
    ensure(entry->next != NULL && entry->prev != NULL);
    return entry;
}

#endif //NETTLE_LIST_C_H
