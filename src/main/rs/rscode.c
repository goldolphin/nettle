/**
 * @author caofuxiang
 *         2015-06-10 13:50:50.
 */

#include "rscode.h"

void rscode_init(rscode_t * rscode, uint32_t k, galois_t * galois, uint32_t * gen_poly) {
    uint32_t n = galois_order(galois)-1;
    uint32_t T = n-k;

    // Construct generator polynomial.
    galois_poly_zero(gen_poly, T);
    gen_poly[0] = galois_element(galois, 0);
    for (uint32_t i = 1; i <= T; ++i) {
        galois_poly_shift_add_scale(galois, gen_poly, i-1, galois_element(galois, i));
    }

    rscode->n = n;
    rscode->k = k;
    rscode->galois = galois;
    rscode->gen_poly = gen_poly;
}

void rscode_prepare(rscode_t * rscode, uint32_t* buffer /* out */) {
    uint32_t T = rscode_checks_num(rscode);
    for (uint32_t i = 0; i < T; ++i) {
        buffer[i] = 0;
    }
}

void rscode_encode(rscode_t * rscode, uint32_t input, uint32_t* checks /* inout */) {
    uint32_t T = rscode_checks_num(rscode);
    uint32_t factor = galois_add(input, checks[0]);
    for (uint32_t i = 0; i < T-1; ++i) {
        // 0 => degree(T-1)
        checks[i] = galois_add(checks[i+1], galois_mul(rscode->galois, rscode->gen_poly[T-1-i], factor));
    }
    checks[T-1] = galois_mul(rscode->galois, rscode->gen_poly[0], factor);
}

void rscode_decode_syndromes(rscode_t * rscode, uint32_t input, uint32_t input_degree, uint32_t * syndromes) {
    uint32_t T = rscode_checks_num(rscode);
    for (uint32_t i = 0; i < T; ++i) {
        syndromes[i] = galois_add(syndromes[i], galois_mul(rscode->galois, input,
                                                           galois_element(rscode->galois, input_degree * (i + 1))));
    }
}

bool rscode_check_syndromes(rscode_t * rscode, uint32_t * syndromes) {
    uint32_t T = rscode_checks_num(rscode);
    for (uint32_t i = 0; i < T; ++i) {
        if (syndromes[i] != 0) return false;
    }
    return true;
}

#define z(i) galois_element(galois, erasure_degrees[i])

uint32_t rscode_decode_erasure(rscode_t * rscode, uint32_t * syndromes, uint32_t * erasure_degrees, uint32_t erasure_num, uint32_t erasure_index) {
    galois_t * galois = rscode->galois;
        uint32_t z_i = z(erasure_index);
        uint32_t prod = z_i;

        // Compute sigma[i,j]
        uint32_t sigma[erasure_num];
        for (uint32_t j = 1; j < erasure_num; ++j) {
            sigma[j] = 0;
        }
        sigma[0] = galois_element(galois, 0);

        for (uint32_t j = 0, d = 0; j < erasure_num; ++j) {
            if (erasure_index != j) {
                galois_poly_shift_add_scale(galois, sigma, d++, z(j));
                prod = galois_mul(galois, prod, galois_add(z_i, z(j)));
            }
        }

        // Solve erasure value.
        uint32_t sum = 0;
        for (uint32_t j = 0; j < erasure_num; ++j) {
            sum = galois_add(sum, galois_mul(galois, syndromes[erasure_num-1-j], sigma[erasure_num-1-j]));
        }
        return galois_div(galois, sum, prod);
}

void rscode_decode_erasures(rscode_t * rscode, uint32_t * syndromes, uint32_t * erasure_degrees, uint32_t erasure_num, uint32_t * erasure_values /* out */) {
    for (uint32_t i = 0; i < erasure_num; ++i) {
        erasure_values[i] = rscode_decode_erasure(rscode, syndromes, erasure_degrees, erasure_num, i);
    }
}
