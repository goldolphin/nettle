/**
 * @author caofuxiang
 *         2015-07-22 20:18:18.
 */

#include "lrumap.h"

struct lrumap_entry_s {
    list_entry_t super;
    pair_t key_value;
};

static inline list_entry_t * lrumap_entry_to_super(lrumap_entry_t * entry) {
    return &entry->super;
}

static inline lrumap_entry_t * lrumap_entry_from_super(list_entry_t * entry) {
    return container_of(entry, lrumap_entry_t, super);
}

void lrumap_init(lrumap_t * map, size_t capacity, double factor /* (0, 1] */, hash_func_t hash_func, equal_func_t equal_func) {
    hashmap_init(&map->map, capacity, factor, hash_func, equal_func);
    list_init(&map->pool);
    list_init(&map->queue);
    map->capacity = capacity;
    map->entries = new_array(lrumap_entry_t, capacity);
    for (int i = 0; i < capacity; ++i) {
        list_push_front(&map->pool, lrumap_entry_to_super(&map->entries[i]));
    }
}

void lrumap_init1(lrumap_t *map, size_t capacity, hash_func_t hash_func, equal_func_t equal_func) {
    lrumap_init(map, capacity, 0.75, hash_func, equal_func);
}

void lrumap_destroy(lrumap_t * map) {
    hashmap_destroy(&map->map);
    free(map->entries);
}

bool lrumap_put(lrumap_t * map, void * key, void * value) {
    if (lrumap_size(map) == map->capacity) {
        return false;
    }
    lrumap_entry_t * entry = lrumap_entry_from_super(list_pop_back(&map->pool));
    pair_set(&entry->key_value, key, value);
    ensure(hashmap_put_if_absent(&map->map, key, entry)); // We don't allow putting a key that already exists in the map.
    list_push_front(&map->queue, lrumap_entry_to_super(entry));
    return true;
}

bool lrumap_get(lrumap_t * map, void * key, pair_t * key_value) {
    pair_t kv;
    if (!hashmap_get(&map->map, key, &kv)) {
        return false;
    }
    lrumap_entry_t * entry = kv.value;
    list_remove(lrumap_entry_to_super(entry));
    list_push_front(&map->queue, lrumap_entry_to_super(entry));
    pair_copy(key_value, &entry->key_value);
    return true;
}

bool lrumap_remove(lrumap_t * map, void * key) {
    pair_t key_value;
    if (!hashmap_remove(&map->map, key, &key_value)) {
        return false;
    }
    lrumap_entry_t * entry = key_value.value;
    list_remove(lrumap_entry_to_super(entry));
    list_push_front(&map->pool, lrumap_entry_to_super(entry));
    return true;
}

/**
 * Returns the eldest key-value pair.
 */
bool lrumap_peek(lrumap_t * map, pair_t * key_value) {
    if (lrumap_size(map) == 0) {
        return false;
    }
    lrumap_entry_t * entry = lrumap_entry_from_super(list_peek_back(&map->queue));
    pair_copy(key_value, &entry->key_value);
    return true;
}

/**
 * Try to make the specified entry become the eldest one.
 */
bool lrumap_make_eldest(lrumap_t * map, void * key) {
    pair_t key_value;
    if (!hashmap_get(&map->map, key, &key_value)) {
        return false;
    }
    lrumap_entry_t * entry = key_value.value;
    list_remove(lrumap_entry_to_super(entry));
    list_push_back(&map->queue, lrumap_entry_to_super(entry));
    return true;
}

lrumap_iterator_t lrumap_begin(lrumap_t * map) {
    return lrumap_entry_from_super(map->queue.tail.prev);
}

lrumap_iterator_t lrumap_end(lrumap_t * map) {
    return lrumap_entry_from_super(&map->queue.head);
}

lrumap_iterator_t lrumap_next(lrumap_t * map, lrumap_iterator_t iter, pair_t * key_value) {
    pair_copy(key_value, &iter->key_value);
    return lrumap_entry_from_super(lrumap_entry_to_super(iter)->prev);
}
