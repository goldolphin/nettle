/**
 * @author caofuxiang
 *         2015-06-09 16:32:32.
 */

#include "galois.h"
#include <utils/sbuilder.h>

void galois_init(galois_t * galois, size_t degree, uint32_t gen_poly, uint32_t * exps, uint32_t * logs) {
    uint32_t order = 0x1U << degree;

    exps[0] = 1;
    exps[order-1] = exps[0];
    logs[0] = 0; // should be -infinite.
    logs[1] = 0;
    for (uint32_t i = 1; i < order-1; ++i) {
        uint32_t h = exps[i-1] >> (degree-1); // get the sign
        uint32_t s = (exps[i-1] << 1);        // a^n = a^(n-1)*X, a shift
        exps[i] = s ^ (gen_poly & (((uint32_t)0) - h)); // a^n mod genpoly: h == 0 ? s : gen_poly^s
        logs[exps[i]] = i;
    }

    galois->degree = degree;
    galois->order = order;
    galois->gen_poly = gen_poly;
    galois->exps = exps;
    galois->logs = logs;
}

void galois_poly_zero(uint32_t * a, size_t degree) {
    for (size_t i = 0; i <= degree; ++i) {
        a[i] = 0;
    }
}

void galois_poly_copy(uint32_t * src, uint32_t * dst, size_t degree) {
    for (size_t i = 0; i <= degree; ++i) {
        dst[i] = src[i];
    }
}

size_t galois_poly_mul(galois_t * galois, uint32_t * a, size_t degree_a, uint32_t * b, size_t degree_b, uint32_t * result) {
    size_t degree_res = degree_a + degree_b;
    galois_poly_zero(result, degree_res);
    for (size_t i = 0; i <= degree_a; ++i) {
        for (size_t j = 0; j <= degree_b; ++j) {
            size_t n = i+j;
            result[n] = galois_add(result[n], galois_mul(galois, a[i], b[j]));
        }
    }
    return degree_res;
}

bool sbuilder_galois_poly(sbuilder_t * builder, galois_t * galois, uint32_t * a, size_t degree) {
    for (int j = (int) degree; j >= 0; --j) {
        if (sbuilder_len(builder) > 0) sbuilder_str(builder, " + ");
        if (a[j] > 0) {
            bool printed = false;
            if (a[j] > 1) {
                uint32_t ai = galois_index(galois, a[j]);
                if (ai == 1) {
                    sbuilder_format(builder, "a");
                } else {
                    sbuilder_format(builder, "a^%d", ai);
                }
                printed = true;
            }
            if (j > 0) {
                if (j == 1) {
                    sbuilder_format(builder, "X");
                } else {
                    sbuilder_format(builder, "X^%d", j);
                }
                printed = true;
            }
            if (!printed) {
                sbuilder_str(builder, "1");
            }
        }
    }
    return true;
}
