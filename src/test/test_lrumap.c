#include <utils/lrumap.h>
#include <utils/sbuilder.h>
#include <utils/pair_t.h>

/**
 * @author caofuxiang
 *         2015-08-04 10:09:09.
 */

void dump_lrumap(lrumap_t * map) {
    SBUILDER(builder, 4096);
    sbuilder_str(&builder, "{");
    for (lrumap_iterator_t i = lrumap_begin(map);i != lrumap_end(map); ) {
        pair_t key_value;
        i = lrumap_next(map, i, &key_value);
        sbuilder_format(&builder, "%d:%d", key_value.key, key_value.value);
        if (i != lrumap_end(map)) {
            sbuilder_str(&builder, ", ");
        }
    }
    sbuilder_str(&builder, "}");
    printf("%s\n", builder.buf);
}

int main() {
    size_t capacity = 10;
    lrumap_t map;
    lrumap_init1(&map, capacity, naive_hash_func, naive_equal_func);
    dump_lrumap(&map);

    printf("lrumap_put()\n");
    for (int i = 0; i < capacity; ++i) {
        lrumap_put(&map, int2ptr(i, void), int2ptr(i, void));
    }
    dump_lrumap(&map);
    pair_t key_value;

    printf("lrumap_get(3)\n");
    lrumap_get(&map, int2ptr(3, void), &key_value);
    dump_lrumap(&map);

    printf("lrumap_peek()\n");
    lrumap_peek(&map, &key_value);
    printf("Eldest entry: key=%d, value=%d\n", ptr2int(key_value.key, int), ptr2int(key_value.value, int));
    dump_lrumap(&map);

    printf("lrumap_remove(4)\n");
    lrumap_remove(&map, int2ptr(4, void));
    dump_lrumap(&map);

    printf("lrumap_make_eldest(6)\n");
    lrumap_make_eldest(&map, int2ptr(6, void));
    dump_lrumap(&map);

    return 0;
}
