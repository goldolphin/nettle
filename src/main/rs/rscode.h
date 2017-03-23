/**
 * @author caofuxiang
 *         2015-06-10 13:50:50.
 */

#ifndef NETTLE_RSCODE_H
#define NETTLE_RSCODE_H

#include "galois.h"

typedef struct {
    uint32_t n; // n == galois.order-1
    uint32_t k; // k < n
    galois_t * galois;
    uint32_t * gen_poly; // size = n-k+1
} rscode_t;

#define RSCODE(v, k, galois) \
rscode_t v; \
uint32_t VNAME(gen_poly_)[galois_order(galois)-(k)]; \
rscode_init(&v, (k), (galois), VNAME(gen_poly_))

void rscode_init(rscode_t * rscode, uint32_t k, galois_t * galois, uint32_t * gen_poly);

static inline uint32_t rscode_checks_num(rscode_t * rscode) {
    return rscode->n - rscode->k;
}

void rscode_prepare(rscode_t * rscode, uint32_t* buffer /* out */);

void rscode_encode(rscode_t * rscode, uint32_t input, uint32_t* checks /* inout */);

void rscode_decode_syndromes(rscode_t * rscode, uint32_t input, uint32_t input_degree, uint32_t * syndromes);

bool rscode_check_syndromes(rscode_t * rscode, uint32_t * syndromes);

uint32_t rscode_decode_erasure(rscode_t * rscode, uint32_t * syndromes, uint32_t * erasure_degrees, uint32_t erasure_num, uint32_t erasure_index);

void rscode_decode_erasures(rscode_t * rscode, uint32_t * syndromes, uint32_t * erasure_degrees, uint32_t erasure_num, uint32_t * erasure_values /* out */);
#endif //NETTLE_RSCODE_H
