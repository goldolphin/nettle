/**
 * @author caofuxiang
 *         2015-07-08 09:40:40.
 */

#include <string.h>
#include "hashmap.h"

struct hashmap_entry_s {
    pair_t key_value;
    bool used;
    hashmap_entry_t * prev;
    hashmap_entry_t * next;
};

struct hashmap_bucket_s {
    hashmap_entry_t * head;
};


static inline void hashmap_reset(hashmap_t * map) {
    map->size = 0;
    for (size_t i = 0; i < map->bucket_num; ++i) {
        map->buckets[i].head = NULL;
    }
    for (size_t i = 0; i < map->capacity; ++i) {
        map->entries[i].used = false;
    }
}

static inline void hashmap_alloc(hashmap_t * map, size_t capacity) {
    map->capacity = capacity;
    map->bucket_num = (size_t) (map->capacity * map->factor);
    map->buckets = new_array(hashmap_bucket_t, map->bucket_num);
    map->entries = new_array(hashmap_entry_t, map->capacity);
}

void hashmap_init(hashmap_t * map, size_t initial_capacity, double factor /* (0, 1] */, hash_func_t hash_func, equal_func_t equal_func) {
    ensure(factor > 0 && factor <= 1.0);
    map->hash_func = hash_func;
    map->equal_func = equal_func;
    map->factor = factor;
    hashmap_alloc(map, initial_capacity);
    hashmap_reset(map);
}

void hashmap_init1(hashmap_t *map, size_t initial_capacity, hash_func_t hash_func, equal_func_t equal_func) {
    hashmap_init(map, initial_capacity, 0.75, hash_func, equal_func);
}

void hashmap_destroy(hashmap_t * map) {
    free(map->buckets);
    free(map->entries);
}

static void hashmap_grow(hashmap_t *map) {
    // Save the snapshot.
    size_t capacity = map->capacity;
    hashmap_bucket_t *buckets = map->buckets;
    hashmap_entry_t *entries = map->entries;

    // Resize the map.
    hashmap_alloc(map, 2*capacity);
    hashmap_reset(map);

    // Copy data.
    for (size_t i = 0; i < capacity; ++i) {
        hashmap_entry_t * entry = &entries[i];
        if (entry->used) {
            hashmap_put(map, entry->key_value.key, entry->key_value.value);
        }
    }

    // Destroy the snapshot.
    free(buckets);
    free(entries);
}

static inline void hashmap_put_new(hashmap_t * map, hashmap_bucket_t * bucket, size_t hash, void * key, void * value) {
    // Ensure capacity.
    while (map->size >= map->bucket_num) {
        hashmap_grow(map);
        bucket = &map->buckets[hash % map->bucket_num]; // We must recalculate bucket address.
    }

    // Allocate a new entry.
    for (size_t i = hash % map->capacity; ; ) {
        hashmap_entry_t * entry = &map->entries[i];
        if (entry->used) {
            i = (i+1) % map->capacity;
            continue;
        }
        pair_set(&entry->key_value, key, value);
        entry->used = true;

        // add to the chain.
        entry->prev = NULL;
        entry->next = bucket->head;
        if (entry->next != NULL) {
            entry->next->prev = entry;
        }
        bucket->head = entry;
        ++ map->size;
        return;
    }
}

/**
 * Return the old value bound with the key(the old value will be replaced), or NULL if there's no such bindings.
 */
void * hashmap_put(hashmap_t * map, void * key, void * value) {
    size_t hash = map->hash_func(key);
    hashmap_bucket_t * bucket = &map->buckets[hash % map->bucket_num];

    // If the entry exists.
    for (hashmap_entry_t * entry = bucket->head; entry != NULL; entry = entry->next) {
        if (map->equal_func(key, entry->key_value.key)) {
            void * old_value = entry->key_value.value;
            entry->key_value.value = value;
            return old_value;
        }
    }

    // Otherwise
    hashmap_put_new(map, bucket, hash, key, value);
    return NULL;
}

bool hashmap_put_if_absent(hashmap_t * map, void * key, void * value) {
    size_t hash = map->hash_func(key);
    hashmap_bucket_t * bucket = &map->buckets[hash % map->bucket_num];

    // If the entry exists.
    for (hashmap_entry_t * entry = bucket->head; entry != NULL; entry = entry->next) {
        if (map->equal_func(key, entry->key_value.key)) {
            return false;
        }
    }

    // Otherwise
    hashmap_put_new(map, bucket, hash, key, value);
    return true;
}

bool hashmap_get(hashmap_t * map, void * key, pair_t * key_value) {
    size_t index = map->hash_func(key) % map->bucket_num;
    hashmap_bucket_t * bucket = &map->buckets[index];
    for (hashmap_entry_t * entry = bucket->head; entry != NULL; entry = entry->next) {
        if (map->equal_func(key, entry->key_value.key)) {
            pair_copy(key_value, &entry->key_value);
            return true;
        }
    }
    return false;
}

bool hashmap_remove(hashmap_t * map, void * key, pair_t * key_value) {
    size_t index = map->hash_func(key) % map->bucket_num;
    hashmap_bucket_t * bucket = &map->buckets[index];
    for (hashmap_entry_t * entry = bucket->head; entry != NULL; entry = entry->next) {
        if (map->equal_func(key, entry->key_value.key)) {
            if (entry->prev != NULL) {
                entry->prev->next = entry->next;
            } else {
                bucket->head = entry->next;
            }
            if (entry->next != NULL) {
                entry->next->prev = entry->prev;
            }
            entry->used = false;
            -- map->size;
            pair_copy(key_value, &entry->key_value);
            return true;
        }
    }
    return false;
}

void hashmap_clear(hashmap_t * map) {
    if (map->size == 0) return;
    hashmap_reset(map);
}

static inline hashmap_iterator_t find_valid_entry(hashmap_t *map, hashmap_iterator_t iter) {
    for (size_t i = iter; i < map->capacity; ++i) {
        if (map->entries[i].used) {
            return i;
        }
    }
    return hashmap_end(map);
}

hashmap_iterator_t hashmap_begin(hashmap_t * map) {
    return find_valid_entry(map, 0);
}

hashmap_iterator_t hashmap_next(hashmap_t * map, hashmap_iterator_t iter, pair_t * key_value) {
    ensure(iter != hashmap_end(map));
    hashmap_entry_t * entry = &map->entries[iter];
    ensure(entry->used);
    pair_copy(key_value, &entry->key_value);
    return find_valid_entry(map, iter + 1);
}

size_t naive_hash_func (void * key) {
    return (size_t) key;
}

bool naive_equal_func (void * key1, void * key2) {
    return key1 == key2;
}

size_t str_hash_func (void * key) {
    size_t h = 0;
    for (char * s = key; s[0] != '\0' ; ++s) {
        h = 31*h + s[0];
    }
    return h;
}

bool str_equal_func (void * key1, void * key2) {
    return strcmp(key1, key2) == 0;
}
