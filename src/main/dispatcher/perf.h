/**
 * @author caofuxiang
 *         2015-07-10 11:23:23.
 */

#ifndef NETTLE_PERF_H
#define NETTLE_PERF_H

#include <utils/sbuilder.h>
#include <utils/hashmap.h>
#include "counter.h"

typedef struct {
    hashmap_t counters;
} perf_t;

void perf_init(perf_t * perf, const char **counter_names, size_t counter_num);

void perf_destroy(perf_t * perf);

void perf_inc(perf_t * perf, const char * name, int n);

uint64_t perf_get(perf_t * perf, const char * name);

bool sbuilder_perf(sbuilder_t * builder, perf_t * perf);

#endif //NETTLE_PERF_H
