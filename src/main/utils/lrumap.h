/**
 * @author caofuxiang
 *         2015-07-22 20:18:18.
 */

#ifndef NETTLE_LRUMAP_H
#define NETTLE_LRUMAP_H

#include "hashmap.h"
#include "list.h"

struct lrumap_entry_s;
typedef struct lrumap_entry_s lrumap_entry_t;
typedef struct lrumap_entry_s * lrumap_iterator_t;

typedef struct {
    hashmap_t map;
    list_t pool;
    list_t queue;
    size_t capacity;
    lrumap_entry_t * entries;
} lrumap_t;

void lrumap_init(lrumap_t * map, size_t capacity, double factor /* (0, 1] */, hash_func_t hash_func, equal_func_t equal_func);

void lrumap_init1(lrumap_t *map, size_t capacity, hash_func_t hash_func, equal_func_t equal_func);

void lrumap_destroy(lrumap_t * map);

static inline size_t lrumap_size(lrumap_t * map) {
    size_t size = hashmap_size(&map->map);
    ensure(size <= map->capacity);
    return size;
}

static inline size_t lrumap_capacity(lrumap_t * map) {
    return map->capacity;
}

bool lrumap_put(lrumap_t * map, void * key, void * value);

bool lrumap_get(lrumap_t * map, void * key, pair_t * key_value);

bool lrumap_remove(lrumap_t * map, void * key);
/**
 * Returns the eldest key-value pair.
 */
bool lrumap_peek(lrumap_t * map, pair_t * key_value);

/**
 * Try to make the specified entry become the eldest one.
 */
bool lrumap_make_eldest(lrumap_t * map, void * key);

lrumap_iterator_t lrumap_begin(lrumap_t * map);

lrumap_iterator_t lrumap_end(lrumap_t * map);

lrumap_iterator_t lrumap_next(lrumap_t * map, lrumap_iterator_t iter, pair_t * key_value);

#endif //NETTLE_LRUMAP_H
