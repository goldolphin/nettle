/**
 * @author caofuxiang
 *         2015-07-08 09:40:40.
 */

#ifndef NETTLE_HASHMAP_H
#define NETTLE_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include "memory.h"
#include "pair_t.h"

struct hashmap_entry_s;
typedef struct hashmap_entry_s hashmap_entry_t;

struct hashmap_bucket_s;
typedef struct hashmap_bucket_s hashmap_bucket_t;

typedef size_t (*hash_func_t) (void * key);
typedef bool (*equal_func_t) (void * key1, void * key2);

typedef size_t hashmap_iterator_t;

/**
 * A hash table
 */
typedef struct {
    size_t capacity;
    double factor;
    hash_func_t hash_func;
    equal_func_t equal_func;
    size_t bucket_num;
    size_t size;
    hashmap_bucket_t * buckets;
    hashmap_entry_t * entries;
} hashmap_t;

void hashmap_init(hashmap_t * map, size_t initial_capacity, double factor /* (0, 1] */, hash_func_t hash_func, equal_func_t equal_func);

/**
 * facotr = 0.75
 */
void hashmap_init1(hashmap_t *map, size_t initial_capacity, hash_func_t hash_func, equal_func_t equal_func);

void hashmap_destroy(hashmap_t * map);

static inline size_t hashmap_size(hashmap_t * map) {
    return map->size;
}

/**
 * Return the old value bound with the key(the old value will be replaced), or NULL if there's no such bindings.
 */
void * hashmap_put(hashmap_t * map, void * key, void * value);

bool hashmap_put_if_absent(hashmap_t * map, void * key, void * value);

bool hashmap_get(hashmap_t * map, void * key, pair_t * key_value);

bool hashmap_remove(hashmap_t * map, void * key, pair_t * key_value);

void hashmap_clear(hashmap_t * map);

hashmap_iterator_t hashmap_begin(hashmap_t * map);

static inline hashmap_iterator_t hashmap_end(hashmap_t * map) {
    return map->capacity;
}

/**
 * Return the next valid iterator, or hashmap_end(map) if there're no more bindings after current iterator.
 */
hashmap_iterator_t hashmap_next(hashmap_t * map, hashmap_iterator_t iter, pair_t * key_value);

/**
 * Hash functions & Equal functions
 */

size_t naive_hash_func (void * key);
bool naive_equal_func (void * key1, void * key2);

size_t str_hash_func (void * key);
bool str_equal_func (void * key1, void * key2);

#endif //NETTLE_HASHMAP_H
