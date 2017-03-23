/**
 * @author caofuxiang
 *         2015-06-24 10:58:58.
 */

#include "rs.h"
#include <utils/memory.h>

#define MIN(a, b) ((a)<(b) ? (a) : (b))

#define T_SIZE(rs) ((rs)->N - (rs)->K)

static inline void bitmap256_reset(bitmap256_t * bitmap) {
    zero_data(bitmap);
}

static inline void bitmap256_set(bitmap256_t * bitmap, int key) {
    int n = key / 64;
    int m = key % 64;
    bitmap->buffer[n] |= (1 << m);
}

static inline bool bitmap256_isset(bitmap256_t * bitmap, int key) {
    int n = key / 64;
    int m = key % 64;
    return (bitmap->buffer[n] & (1 << m)) != 0;
}

void rs_init(rs_t *rs, int N, int K, int max_data_size) {
    galois256_init(&rs->galois, rs->galois_exps, rs->galois_logs);
    uint32_t rs_K = (uint32_t) (RS_GALOIS_ORDER-1-(N-K));
    rscode_init(&rs->rscode, rs_K, &rs->galois, rs->rs_gen_poly);
    rs->N = N;
    rs->K = K;
    rs->max_data_size = max_data_size;
}

void rs_reset_ec(rs_t *rs, rs_ec_t *ec) {
    int T = T_SIZE(rs);
    int max_payload_size = RS_MAX_ENCODED_SIZE(rs->max_data_size);
    ec->count = 0;
    zero_array(ec->checks, max_payload_size*T);
}

void rs_init_ec(rs_t *rs, rs_ec_t *ec) {
    int T = T_SIZE(rs);
    int max_payload_size = RS_MAX_ENCODED_SIZE(rs->max_data_size);
    ec->checks = new_array(uint32_t, max_payload_size*T);
    rs_reset_ec(rs, ec);
}

void rs_destroy_ec(rs_ec_t *ec) {
    free(ec->checks);
}

void rs_reset_dc(rs_t *rs, rs_dc_t *dc) {
    int T = T_SIZE(rs);
    int max_payload_size = RS_MAX_ENCODED_SIZE(rs->max_data_size);
    dc->count = 0;
    bitmap256_reset(&dc->mask);
    zero_array(dc->syndromes, max_payload_size*T);
    zero_array(dc->erasure_degrees, T);
    dc->erasure_num = 0;
    dc->raw_erasure_num = 0;
}

void rs_init_dc(rs_t *rs, rs_dc_t *dc) {
    int T = T_SIZE(rs);
    int max_payload_size = RS_MAX_ENCODED_SIZE(rs->max_data_size);
    dc->syndromes = new_array(uint32_t, max_payload_size*T);
    dc->erasure_degrees = new_array(uint32_t, T);
    rs_reset_dc(rs, dc);
}

void rs_destroy_dc(rs_dc_t *dc) {
    free(dc->syndromes);
    free(dc->erasure_degrees);
}

static inline int build_redundant(uint8_t * to, uint32_t * from, int size, int T, int j) {
    int valid_size = 0;
    int trailing_zero = 0;
    for (int i = 0; i < size; ++i) {
        to[i] = (uint8_t) from[i*T+j];
        if (to[i] == 0) {
            trailing_zero ++;
        } else {
            valid_size += 1 + trailing_zero;
            trailing_zero = 0;
        }
    }
    return valid_size;
}

int rs_encode(rs_t *rs, rs_ec_t *ec, uint8_t *input, int size, int *degree /* out */) {
    if (size < 0 || size > rs->max_data_size) {
        return E_RS_INVALID_SIZE;
    }

    int T = T_SIZE(rs);
    *degree = rs->N-1-ec->count;

    // Encode size info.
    for (int i = 0; i < RS_HEADER_SIZE; ++i) {
        rscode_encode(&rs->rscode, (uint8_t)(0xff & (size >> (i * 8))), ec->checks + i*T);
    }

    // Encode data.
    int offset = RS_HEADER_SIZE;
    for (size_t i = 0; i < size; ++i) {
        rscode_encode(&rs->rscode, input[i], ec->checks + (i + offset)*T);
    }

    // Encode padding zeros.
    offset += size;
    int padding_size = rs->max_data_size - size;
    for (int i = 0; i < padding_size; ++i) {
        rscode_encode(&rs->rscode, 0, ec->checks + (i + offset)*T);
    }
    ec->count++;

    // Process redundant data.
    if (!rs_is_ec_complete(rs, ec)) {
        return 0;
    }

    return T;
}

int rs_encode_result(rs_t *rs, rs_ec_t *ec, int index, uint8_t *buffer, int *size /* inout */, int *degree /* out */) {
    int T = T_SIZE(rs);
    int max_payload_size = RS_MAX_ENCODED_SIZE(rs->max_data_size);
    if (index < 0 || index >= T) {
        return E_RS_INVALID_INDEX;
    }
    if (*size < max_payload_size) {
        return E_RS_INVALID_SIZE;
    }
    int s = build_redundant(buffer, ec->checks, max_payload_size, T, index);
    *size = s;
    *degree = T-1-index;
    return 0;
}

#define IS_RAW(degree) ((degree) >= T)

int rs_decode(rs_t *rs, rs_dc_t *dc, uint8_t *input, int size, int degree, bool *is_raw /* out */) {
    if (degree < 0 || degree >= rs->N) {
        return E_RS_INVALID_DEGREE;
    }

    int T = T_SIZE(rs);
    bool is_raw0 = IS_RAW(degree);

    int allowed_size = rs->max_data_size;
    if (!is_raw0) allowed_size += RS_HEADER_SIZE;

    if (size < 0 || size > allowed_size) {
        return E_RS_INVALID_SIZE;
    }

    *is_raw = is_raw0;

    // Duplicate packet.
    if (bitmap256_isset(&dc->mask, degree)) {
        return E_RS_DUPLICATE;
    }

    int offset = 0;
    if (is_raw0) {
        // Decode size info.
        for (int i = 0; i < RS_HEADER_SIZE; ++i) {
            rscode_decode_syndromes(&rs->rscode, (uint8_t)(0xff & (size >> (i * 8))), (uint32_t) degree, dc->syndromes + i*T);
        }
        offset = RS_HEADER_SIZE;
    }

    // Decode data.
    for (size_t i = 0; i < size; ++i) {
        rscode_decode_syndromes(&rs->rscode, input[i], (uint32_t) degree, dc->syndromes + (i + offset)*T);
    }
    dc->count++;
    bitmap256_set(&dc->mask, degree);

    // Try to restore missing packets.
    if (!rs_is_dc_complete(rs, dc)) {
        return 0;
    }

    // Build erasure degrees.
    uint32_t erasure_num = 0;
    for (int i = rs->N-1; i >= T; --i) {
        if (!bitmap256_isset(&dc->mask, i)) {
            dc->erasure_degrees[erasure_num++] = (uint32_t) i;
        }
    }
    dc->raw_erasure_num = erasure_num;

    if (erasure_num == 0) {
        dc->erasure_num = erasure_num;
        return 0;
    }

    for (int i = T-1; i >= 0; --i) {
        if (!bitmap256_isset(&dc->mask, i)) {
            dc->erasure_degrees[erasure_num++] = (uint32_t) i;
        }
    }
    dc->erasure_num = erasure_num;

    return dc->raw_erasure_num;
}

int rs_decode_result(rs_t *rs, rs_dc_t *dc, int index, uint8_t *buffer, int *size /* inout */, int *degree /* out */) {
    if (index < 0 || index >= dc->raw_erasure_num) {
        return E_RS_INVALID_INDEX;
    }

    if (*size < rs->max_data_size) {
        return E_RS_INVALID_SIZE;
    }

    int T = T_SIZE(rs);
    int decoded_size = 0;
    for (int i = 0; i < RS_HEADER_SIZE; ++i) {
        uint32_t s = rscode_decode_erasure(&rs->rscode, dc->syndromes + i*T, dc->erasure_degrees, dc->erasure_num,
                                                  (uint32_t) index);
        decoded_size |= s << (i*8);
    }

    if (decoded_size > rs->max_data_size) {
        return E_RS_INVALID_HEADER;
    }

    for (int i = 0; i < decoded_size; ++i) {
        buffer[i] = (uint8_t) rscode_decode_erasure(&rs->rscode, dc->syndromes + (i+ RS_HEADER_SIZE)*T, dc->erasure_degrees, dc->erasure_num,
                                                    (uint32_t) index);
    }

    *size = decoded_size;
    *degree = dc->erasure_degrees[index];

    return 0;
}
