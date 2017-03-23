/**
 * @author caofuxiang
 *         2015-06-26 16:06:06.
 */

#ifndef NETTLE_NT_RS_FEC_H
#define NETTLE_NT_RS_FEC_H

#include <rs/rs.h>
#include <utils/lrumap.h>
#include <utils/utils.h>
#include <dispatcher/perf.h>
#include "nt_fec.h"

typedef struct {
    uint32_t block_id;
    rs_ec_t buffer;
} encode_buffer_t;

typedef struct {
    uint32_t block_id;
    rs_dc_t buffer;
    long timestamp;
} decode_buffer_t;

typedef struct {
    nt_fec_t super;
    rs_t rs;
    idgen_t * idgen;
    encode_buffer_t encode_buffer;
    lrumap_t decode_buffer_cache;
    decode_buffer_t * decode_buffer_pool;
    int decode_buffer_size;
    long decode_timeout;
    perf_t * perf;
} nt_rs_fec_t;

void nt_rs_fec_init(nt_rs_fec_t * fec, uint8_t N, uint8_t K, int max_data_size, int decode_buffer_size, long decode_timeout, idgen_t * idgen, perf_t * perf);

void nt_rs_fec_destroy(nt_rs_fec_t * fec);

#endif //NETTLE_NT_RS_FEC_H
