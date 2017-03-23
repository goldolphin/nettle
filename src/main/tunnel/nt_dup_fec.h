/**
 * @author caofuxiang
 *         2015-07-06 10:45:45.
 */

#ifndef NETTLE_NT_DUP_FEC_H
#define NETTLE_NT_DUP_FEC_H

#include <utils/lrumap.h>
#include <dispatcher/perf.h>
#include <utils/utils.h>
#include "nt_fec.h"

typedef struct {
    nt_fec_t super;
    uint8_t N;
    idgen_t * idgen;
    lrumap_t dedup_cache;
    long dedup_timeout;
    perf_t * perf;
} nt_dup_fec_t;

void nt_dup_fec_init(nt_dup_fec_t * fec, uint8_t N, int dedup_cache_size /* set to 0 to disable deduplication*/, long dedup_timeout, idgen_t * idgen, perf_t * perf);

void nt_dup_fec_destroy(nt_dup_fec_t * fec);

#endif //NETTLE_NT_DUP_FEC_H
