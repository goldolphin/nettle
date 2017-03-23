/**
 * @author caofuxiang
 *         2015-06-26 16:06:06.
 */

#include <utils/memory.h>
#include <utils/utils.h>
#include <rs/rs.h>
#include <dispatcher/common.h>
#include <utils/pair_t.h>
#include "nt_rs_fec.h"
#include "nt_perf.h"
#include "nt_fec.h"

static inline void reset_encode_buffer(nt_rs_fec_t * fec) {
    fec->encode_buffer.block_id = idgen_next(fec->idgen);
    rs_reset_ec(&fec->rs, &fec->encode_buffer.buffer);
}

static inline void reset_decode_buffer(nt_rs_fec_t * fec, decode_buffer_t * decode_buffer, uint32_t block_id) {
    decode_buffer->block_id = block_id;
    rs_reset_dc(&fec->rs, &decode_buffer->buffer);
    decode_buffer->timestamp = 0;
}

static error_t nt_rs_fec_encode(nt_rs_fec_t * fec, uint8_t *input, int size, void * extra, nt_fec_on_encode_t callback) {
    int degree;
    int n = rs_encode(&fec->rs, &fec->encode_buffer.buffer, input, size, &degree);
    ensure(n >= 0); // Should not fail.
    nt_fec_header_t header;
    int max_degree = fec->rs.N-1;
    nt_fec_header_init(&header, NT_RS_FEC_TAG, (uint8_t) fec->rs.N, (uint8_t) fec->rs.K, fec->encode_buffer.block_id, (uint8_t) (max_degree-degree),
                       (uint16_t) size);
    error_t err = callback(extra, &header, input, size);
    if (err != 0) return err;
    if (n == 0) {
        return 0;
    }
    uint8_t encoded[RS_MAX_ENCODED_SIZE(fec->rs.max_data_size)];
    for (int i = 0; i < n; ++i) {
        int encoded_size = sizeof encoded;
        if (rs_encode_result(&fec->rs, &fec->encode_buffer.buffer, i, encoded, &encoded_size, &degree) < 0) {
            // Should not fail.
            err = E_FEC_INTERNAL_ERROR;
            break;
        }
        nt_fec_header_init(&header, NT_RS_FEC_TAG, (uint8_t) fec->rs.N, (uint8_t) fec->rs.K, fec->encode_buffer.block_id, (uint8_t) (max_degree-degree),
                           (uint16_t) encoded_size);
        err = callback(extra, &header, encoded, encoded_size);
        if (err != 0) break;
    }
    reset_encode_buffer(fec);
    return err;
}

static error_t nt_rs_fec_decode(nt_rs_fec_t * fec, nt_fec_header_t * header, uint8_t *input, int size, void * extra, nt_fec_on_decode_t callback) {
    if (header->tag != NT_RS_FEC_TAG || header->N != fec->rs.N || header->K != fec->rs.K) {
        return E_FEC_INVALID_HEADER;
    }

    long current = current_millis();
    uint32_t block_id = nt_fec_header_block_id(header);
    decode_buffer_t * decode_buffer = NULL;
    pair_t key_value;
    if (lrumap_get(&fec->decode_buffer_cache, uint2ptr(block_id, void), &key_value)) {
        decode_buffer = key_value.value;
        if (current - decode_buffer->timestamp <= fec->decode_timeout) {
            // We have the entry
            if (rs_is_dc_complete(&fec->rs, &decode_buffer->buffer)) {
                // All messages in this block are already processed.
                return 0;
            }
        } else {
            // We have the entry, but it's expired.
            if (!rs_is_dc_complete(&fec->rs, &decode_buffer->buffer) && decode_buffer->timestamp > 0) {
                perf_inc(fec->perf, COUNTER_DECODER_BLOCK_EXPIRE, 1);
            }
            reset_decode_buffer(fec, decode_buffer, block_id);
        }
    } else {
        // Expires the oldest data.
        ensure(lrumap_peek(&fec->decode_buffer_cache, &key_value));
        decode_buffer = key_value.value;
        if (!rs_is_dc_complete(&fec->rs, &decode_buffer->buffer) && decode_buffer->timestamp > 0) {
            if (current - decode_buffer->timestamp <= fec->decode_timeout) {
                return E_FEC_DECODER_BUFFER_FULL;
            }
            perf_inc(fec->perf, COUNTER_DECODER_BLOCK_EXPIRE, 1);
        }
        ensure(lrumap_remove(&fec->decode_buffer_cache, key_value.key));
        ensure(lrumap_put(&fec->decode_buffer_cache, uint2ptr(block_id, void), decode_buffer));
        reset_decode_buffer(fec, decode_buffer, block_id);
    }
    if (decode_buffer->timestamp == 0) {
        perf_inc(fec->perf, COUNTER_DECODER_BLOCK_COUNT, 1);
    }

    bool is_raw;
    int max_degree = fec->rs.N-1;
    int n = rs_decode(&fec->rs, &decode_buffer->buffer, input, size, max_degree - header->block_index, &is_raw);
    if (n == E_RS_DUPLICATE) {
        return 0;
    } else if (n < 0) {
        return EINVAL;
    }
    if (rs_is_dc_complete(&fec->rs, &decode_buffer->buffer)) {
        if (n == 0) {
            perf_inc(fec->perf, COUNTER_DECODER_BLOCK_COMPLETE, 1);
        } else {
            perf_inc(fec->perf, COUNTER_DECODER_BLOCK_RESTORE, 1);
            perf_inc(fec->perf, COUNTER_DECODER_RESTORE, n);
        }
    }
    decode_buffer->timestamp = current;

    if (is_raw) {
        error_t err = callback(extra, input, size);
        perf_inc(fec->perf, COUNTER_DECODER_RAW, 1);
        if (err != 0) return err;
    }
    if (n == 0) return 0;
    uint8_t decoded[fec->rs.max_data_size];
    for (int i = 0; i < n; ++i) {
        int decoded_size = sizeof decoded;
        int degree;
        if (rs_decode_result(&fec->rs, &decode_buffer->buffer, i, decoded, &decoded_size, &degree) < 0) {
            // Should not fail.
            return E_FEC_INTERNAL_ERROR;
        }
        error_t err = callback(extra, decoded, decoded_size);
        if (err != 0) return err;
    }
    return 0;
}

void nt_rs_fec_init(nt_rs_fec_t * fec, uint8_t N, uint8_t K, int max_data_size, int decode_buffer_size, long decode_timeout, idgen_t * idgen, perf_t * perf) {
    nt_fec_init(&fec->super, (nt_fec_encode_t) nt_rs_fec_encode, (nt_fec_decode_t) nt_rs_fec_decode);
    rs_init(&fec->rs, N, K, max_data_size);
    rs_init_ec(&fec->rs, &fec->encode_buffer.buffer);
    lrumap_init1(&fec->decode_buffer_cache, (size_t) decode_buffer_size, naive_hash_func, naive_equal_func);
    fec->decode_buffer_pool = new_array(decode_buffer_t, decode_buffer_size);
    for (int i = 0; i < decode_buffer_size; ++i) {
        rs_init_dc(&fec->rs, &fec->decode_buffer_pool[i].buffer);
    }

    fec->decode_buffer_size = decode_buffer_size;
    fec->decode_timeout = decode_timeout;
    fec->idgen = idgen;
    fec->perf = perf;

    reset_encode_buffer(fec);
    for (int i = 0; i < decode_buffer_size; ++i) {
        reset_decode_buffer(fec, &fec->decode_buffer_pool[i], (uint32_t) i);
        lrumap_put(&fec->decode_buffer_cache, uint2ptr(fec->decode_buffer_pool[i].block_id, void), &fec->decode_buffer_pool[i]);
    }
}

void nt_rs_fec_destroy(nt_rs_fec_t * fec) {
    rs_destroy_ec(&fec->encode_buffer.buffer);
    for (int i = 0; i < fec->decode_buffer_size; ++i) {
        rs_destroy_dc(&fec->decode_buffer_pool[i].buffer);
    }
    free(fec->decode_buffer_pool);
    lrumap_destroy(&fec->decode_buffer_cache);
}
