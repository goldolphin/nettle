/**
 * @author caofuxiang
 *         2015-07-10 11:23:23.
 */

#include <utils/pair_t.h>
#include "perf.h"

void perf_init(perf_t * perf, const char **counter_names, size_t counter_num) {
    hashmap_init1(&perf->counters, counter_num, str_hash_func, str_equal_func);
    for (size_t i = 0; i < counter_num; ++i) {
        counter_t * counter = new_data(counter_t);
        counter_init(counter);
        ensure(hashmap_put(&perf->counters, (void *) counter_names[i], counter) == NULL);
    }
}

void perf_destroy(perf_t * perf) {
    for (hashmap_iterator_t i = hashmap_begin(&perf->counters); i != hashmap_end(&perf->counters); ) {
        pair_t key_value;
        i = hashmap_next(&perf->counters, i, &key_value);
        free(key_value.key);
        free(key_value.value);
    }
    hashmap_destroy(&perf->counters);
}

void perf_inc(perf_t * perf, const char * name, int n) {
    pair_t key_value;
    ensure(hashmap_get(&perf->counters, (void *) name, &key_value));
    counter_inc(key_value.value, n);
}

uint64_t perf_get(perf_t * perf, const char * name) {
    pair_t key_value;
    ensure(hashmap_get(&perf->counters, (void *) name, &key_value));
    return counter_value(key_value.value);
}

bool sbuilder_perf(sbuilder_t * builder, perf_t * perf) {
    sbuilder_str(builder, "[");
    for (hashmap_iterator_t iter = hashmap_begin(&perf->counters); iter != hashmap_end(&perf->counters); ) {
        pair_t key_value;
        iter = hashmap_next(&perf->counters, iter, &key_value);
        sbuilder_format(builder, "{\"metric\":\"%s\",\"value\":%lu,\"counterType\":\"COUNTER\"}", key_value.key, counter_value(key_value.value));
        if (iter != hashmap_end(&perf->counters)) {
            sbuilder_str(builder, ", ");
        }
    }
    return sbuilder_str(builder, "]");
}
