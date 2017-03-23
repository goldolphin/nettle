/**
 * @author caofuxiang
 *         2015-07-08 14:37:37.
 */

#ifndef NETTLE_CONFIG_H
#define NETTLE_CONFIG_H

#include <stdbool.h>
#include "hashmap.h"
#include "sbuilder.h"

typedef struct {
    hashmap_t map;
} config_t;

void config_init(config_t * config);

void config_destroy(config_t * config);

bool config_load_from_file(config_t * config, const char * path);

const char * config_get_default(config_t * config, const char * key, const char * default_value);

const char * config_get(config_t * config, const char * key);

bool sbuilder_config(sbuilder_t * builder, config_t * config);

#endif //NETTLE_CONFIG_H
