/**
 * @author caofuxiang
 *         2015-07-08 14:37:37.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "config.h"
#include "utils.h"
#include "pair_t.h"

void config_init(config_t * config) {
    hashmap_init1(&config->map, 8, str_hash_func, str_equal_func);
}

void config_destroy(config_t * config) {
    for (hashmap_iterator_t i = hashmap_begin(&config->map); i != hashmap_end(&config->map); ) {
        pair_t key_value;
        i = hashmap_next(&config->map, i, &key_value);
        free(key_value.key);
        free(key_value.value);
    }
    hashmap_destroy(&config->map);
}

#define SPACES " \t\r\n"

bool config_load_from_file(config_t * config, const char * path) {
    FILE * f = fopen(path, "r");
    if (f == NULL) return false;
    while (true) {
        char line[1024];
        if (fgets(line, sizeof line, f) == NULL) {
            break;
        }
        const char * s = skip(SPACES, line);
        if (s[0] == '#' || s[0] == '\0') {
            continue;
        }
        char key[1025];
        char value[1025];
        if (sscanf(s, "%1024s = %1024s", key, value) < 2) {
            break;
        }
        char * k = strdup(key);
        char * v = strdup(value);
        hashmap_put(&config->map, k, v);
    }
    fclose(f);
    return true;
}

const char * config_get_default(config_t * config, const char * key, const char * default_value) {
    pair_t key_value;
    if (hashmap_get(&config->map, (void *) key, &key_value)) {
        return key_value.value;
    }
    return default_value;
}

const char * config_get(config_t * config, const char * key) {
    return config_get_default(config, key, NULL);
}

bool sbuilder_config(sbuilder_t * builder, config_t * config) {
    for (hashmap_iterator_t i = hashmap_begin(&config->map); i != hashmap_end(&config->map); ) {
        pair_t key_value;
        i = hashmap_next(&config->map, i, &key_value);
        sbuilder_format(builder, "%s = %s\n", key_value.key, key_value.value);
    }
    return true;
}