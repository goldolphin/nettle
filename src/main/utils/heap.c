/**
 * @author caofuxiang
 *         2015-05-12 11:06:06.
 */

#include <stdlib.h>
#include "heap.h"

typedef struct {
    long key;
    void * value;
} entry_t;

struct heap_s {
    size_t capacity;
    size_t size;
    entry_t buffer[0];
};

#define left(n) (2*(n)+1)
#define right(n) (2*(n)+2)
#define parent(n) (((n)-1)/2)

heap_t * make_heap(size_t capacity) {
    heap_t * heap = (heap_t *)malloc(sizeof(heap_t) + capacity*sizeof(entry_t));
    heap->capacity = capacity;
    heap->size = 0;
    return heap;
}

void destroy_heap(heap_t * heap) {
    free(heap);
}

void shift_down(heap_t * heap, size_t n) {
    size_t size = heap->size;
    while (true) {
        size_t l = left(n);
        if (l >= size) {
            break;
        }

        size_t r = right(n);
        size_t mc = l;
        entry_t mc_entry = heap->buffer[l];
        if (r < size && heap->buffer[r].key < mc_entry.key) {
            mc = r;
            mc_entry = heap->buffer[r];
        }

        if (heap->buffer[n].key <= mc_entry.key) {
            break;
        }

        heap->buffer[mc] = heap->buffer[n];
        heap->buffer[n] = mc_entry;
        n = mc;
    }
}

void shift_up(heap_t * heap, size_t n) {
    while (n > 0) {
        size_t p = parent(n);
        entry_t p_entry = heap->buffer[p];
        if (p_entry.key <= heap->buffer[n].key) {
            break;
        }

        heap->buffer[p] = heap->buffer[n];
        heap->buffer[n] = p_entry;
        n = p;
    }
}

size_t heap_capacity(heap_t * heap) {
    return heap->capacity;
}

size_t heap_size(heap_t * heap) {
    return heap->size;
}

bool heap_head(heap_t * heap, long *key, void ** value) {
    if (heap->size == 0) {
        return false;
    }
    if (key != NULL) {
        *key = heap->buffer[0].key;
    }
    if (value != NULL) {
        *value = heap->buffer[0].value;
    }
    return true;
}

bool heap_push(heap_t * heap, long key, void * value) {
    size_t size = heap->size;
    if (size >= heap->capacity) {
        return false;
    }
    heap->size ++;
    heap->buffer[size].key = key;
    heap->buffer[size].value = value;
    shift_up(heap, size);
    return true;
}

bool heap_pop(heap_t * heap, long *key, void ** value) {
    if (!heap_head(heap, key, value)) {
        return false;
    }
    heap->size --;
    heap->buffer[0] = heap->buffer[heap->size];
    shift_down(heap, 0);
    return true;
}
