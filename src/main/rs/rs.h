/**
 * @author caofuxiang
 *         2015-06-24 10:58:58.
 */

#ifndef NETTLE_RS_H
#define NETTLE_RS_H

#include <utils/memory.h>
#include "rscode.h"

#define RS_HEADER_SIZE 4

#define RS_MAX_ENCODED_SIZE(max_data_size) ((max_data_size) + RS_HEADER_SIZE)

#define RS_GALOIS_ORDER 256

/**
 * Error definitions.
 */
#define E_RS_INVALID_SIZE (-1)
#define E_RS_INVALID_INDEX (-2)
#define E_RS_INVALID_DEGREE (-3)
#define E_RS_INVALID_HEADER (-4)
#define E_RS_DUPLICATE (-5)

/**
 * A simple bitmap.
 */
typedef struct {
    uint64_t buffer[4];
} bitmap256_t;

/**
 * Context for encoding.
 */
typedef struct {
    int count;
    uint32_t * checks;
} rs_ec_t;

/**
 * Context for decoding.
 */
typedef struct {
    int count;
    bitmap256_t mask;
    uint32_t * syndromes;
    uint32_t * erasure_degrees;
    uint32_t erasure_num;
    uint32_t raw_erasure_num;
} rs_dc_t;

/**
 * The RS encoder/decoder.
 */
typedef struct {
    int N;
    int K;
    int max_data_size;
    rscode_t rscode;
    galois_t galois;
    uint32_t galois_exps[RS_GALOIS_ORDER];
    uint32_t galois_logs[RS_GALOIS_ORDER];
    uint32_t rs_gen_poly[RS_GALOIS_ORDER-1];
} rs_t;

void rs_init(rs_t *rs, int N, int K, int max_data_size);

void rs_init_ec(rs_t *rs, rs_ec_t *ec);

void rs_init_dc(rs_t *rs, rs_dc_t *dc);

void rs_reset_ec(rs_t *rs, rs_ec_t *ec);

void rs_reset_dc(rs_t *rs, rs_dc_t *dc);

static inline bool rs_is_ec_complete(rs_t *rs, rs_ec_t *ec) {
    ensure(ec->count <= rs->K);
    return ec->count == rs->K;
}

static inline bool rs_is_dc_complete(rs_t *rs, rs_dc_t *dc) {
    ensure(dc->count <= rs->K);
    return dc->count == rs->K;
}

void rs_destroy_ec(rs_ec_t *ec);

void rs_destroy_dc(rs_dc_t *dc);

int rs_encode(rs_t *rs, rs_ec_t *ec, uint8_t *input, int size, int *degree /* out */);

int rs_encode_result(rs_t *rs, rs_ec_t *ec, int index, uint8_t *buffer, int *size /* inout */, int *degree /* out */);

int rs_decode(rs_t *rs, rs_dc_t *dc, uint8_t *input, int size, int degree, bool *is_raw /* out */);

int rs_decode_result(rs_t *rs, rs_dc_t *dc, int index, uint8_t *buffer, int *size /* inout */, int *degree /* out */);

#endif //NETTLE_RS_H
