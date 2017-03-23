/**
 * @author caofuxiang
 *         2015-06-09 16:32:32.
 */

#ifndef NETTLE_GALOIS_H
#define NETTLE_GALOIS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * A binary Galois Field.
 */
typedef struct {
    size_t degree; // <= 32
    uint32_t order;  // = pow(2, degree)
    uint32_t gen_poly; // bits = degree+1
    uint32_t * exps; // size = order
    uint32_t * logs; // size = order
} galois_t;

void galois_init(galois_t * galois, size_t degree, uint32_t gen_poly, uint32_t * exps, uint32_t * logs);

static inline void galois16_init(galois_t * galois, uint32_t * exps, uint32_t * logs) {
    galois_init(galois, 4, 0b10011, exps, logs);
}

static inline void galois256_init(galois_t * galois, uint32_t * exps, uint32_t * logs) {
    galois_init(galois, 8, 0b100011101, exps, logs);
}

#ifndef VNAME
#define CONCATE0(x, y) x##y
#define CONCATE(x, y) CONCATE0(x, y)
#define VNAME(x) CONCATE(x, __LINE__)
#endif

#define GALOIS(v, degree, gen_poly) \
galois_t v; \
uint32_t VNAME(exps_)[0x1U <<(degree)]; \
uint32_t VNAME(logs_)[0x1U <<(degree)]; \
galois_init(&v, (degree), (gen_poly), VNAME(exps_), VNAME(logs_))

/**
 * A predefined GF(2^4) galois field.
 */
#define GALOIS16(v) GALOIS(v, 4, 0b10011)

/**
 * A predefined GF(2^8) galois field.
 */
#define GALOIS256(v) GALOIS(v, 8, 0b100011101)

static inline uint32_t galois_order(galois_t * galois) {
    return galois->order;
}

static inline uint32_t galois_element(galois_t *galois, uint32_t i) {
    return galois->exps[i % (galois->order-1)];
}

static inline uint32_t galois_index(galois_t *galois, uint32_t i) {
    return galois->logs[i];
}

static inline uint32_t galois_add(uint32_t a, uint32_t b) {
    return a ^ b;
}

static inline uint32_t galois_mul(galois_t * galois, uint32_t a, uint32_t b) {
    if (a == 0 || b == 0) return 0;
    return galois_element(galois, galois->logs[a] + galois->logs[b]);
}

static inline uint32_t galois_div(galois_t * galois, uint32_t a, uint32_t b) {
    if (a == 0 || b == 0) return a/b;
    return galois_element(galois, galois->order - 1 + galois->logs[a] - galois->logs[b]);
}

static inline uint32_t galois_inv(galois_t * galois, uint32_t a) {
    return galois_element(galois, galois->order - 1 - galois->logs[a]);
}

static inline bool galois_in(galois_t * galois, uint32_t a) {
    return a < galois->order;
}

void galois_poly_zero(uint32_t * a, size_t degree);

static inline size_t galois_poly_shift_add_scale(galois_t * galois, uint32_t * dst, size_t degree, uint32_t scale) {
    size_t degree_res = degree + 1;
    dst[degree_res] = 0;
    for (size_t i = degree_res; i > 0; --i) {
        dst[i] = galois_add(dst[i-1], galois_mul(galois, dst[i], scale));
    }
    dst[0] = galois_mul(galois, dst[0], scale);
    return degree_res;
}

void galois_poly_copy(uint32_t * src, uint32_t * dst, size_t degree);

size_t galois_poly_mul(galois_t * galois, uint32_t * a, size_t degree_a, uint32_t * b, size_t degree_b, uint32_t * result);

struct sbuilder_s;
bool sbuilder_galois_poly(struct sbuilder_s * builder, galois_t * galois, uint32_t * a, size_t degree);

#endif //NETTLE_GALOIS_H
