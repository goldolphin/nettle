/**
 * @author caofuxiang
 *         2015-07-06 10:45:45.
 */

#include <utils/utils.h>
#include <dispatcher/common.h>
#include <utils/pair_t.h>
#include "nt_dup_fec.h"

static error_t nt_dup_fec_encode(nt_dup_fec_t * fec, uint8_t *input, int size, void * extra, nt_fec_on_encode_t callback) {
    nt_fec_header_t header;
    uint32_t block_id = idgen_next(fec->idgen);
    for (uint8_t i = 0; i < fec->N; ++i) {
        nt_fec_header_init(&header, NT_DUP_FEC_TAG, fec->N, 1, block_id, i, (uint16_t) size);
        error_t err = callback(extra, &header, input, size);
        if (err != 0) return err;
    }
    return 0;
}

static error_t nt_dup_fec_decode(nt_dup_fec_t * fec, nt_fec_header_t * header, uint8_t *input, int size, void * extra, nt_fec_on_decode_t callback) {
    if (header->tag != NT_DUP_FEC_TAG || header->K != 1) { // We don't care N in dup fec.
        return E_FEC_INVALID_HEADER;
    }

    uint32_t block_id = nt_fec_header_block_id(header);
    if (lrumap_capacity(&fec->dedup_cache) > 0) {
        long current = current_millis();
        pair_t key_value;
        if (lrumap_get(&fec->dedup_cache, uint2ptr(block_id, void), &key_value)) {
            if (current - ptr2int(key_value.value, long) > fec->dedup_timeout) {
                // We have the entry, but it's expired.
                ensure(lrumap_remove(&fec->dedup_cache, uint2ptr(block_id, void)));
            } else {
                // We have the entry, duplicate.
                return 0;
            }
        } else {
            // Expires the oldest data.
            if (lrumap_size(&fec->dedup_cache) == lrumap_capacity(&fec->dedup_cache)) {
                ensure(lrumap_peek(&fec->dedup_cache, &key_value));
                ensure(lrumap_remove(&fec->dedup_cache, key_value.key));
            }
        }

        // Insert new entry.
        ensure(lrumap_put(&fec->dedup_cache, uint2ptr(block_id, void), int2ptr(current, void)));
    }

    return callback(extra, input, size);
}

void nt_dup_fec_init(nt_dup_fec_t * fec, uint8_t N, int dedup_cache_size /* set to 0 to disable deduplication*/, long dedup_timeout, idgen_t * idgen, perf_t * perf) {
    nt_fec_init(&fec->super, (nt_fec_encode_t) nt_dup_fec_encode, (nt_fec_decode_t) nt_dup_fec_decode);
    lrumap_init1(&fec->dedup_cache, (size_t) dedup_cache_size, naive_hash_func, naive_equal_func);

    fec->N = N;
    fec->dedup_timeout = dedup_timeout;
    fec->idgen = idgen;
    fec->perf = perf;
}

void nt_dup_fec_destroy(nt_dup_fec_t * fec) {
    lrumap_destroy(&fec->dedup_cache);
}
