/**
 * @author caofuxiang
 *         2015-07-23 10:37:37.
 */

#include "list.h"

void list_init(list_t * list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;

    list->tail.prev = &list->head;
    list->tail.next = NULL;
}
