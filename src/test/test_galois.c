/**
 * @author caofuxiang
 *         2015-06-26 10:40:40.
 */

#include <assert.h>
#include <utils/sbuilder.h>
#include <rs/galois.h>

void test(galois_t * galois) {
    for (uint32_t a = 0; a < galois->order; ++a) {
        for (uint32_t b = 0; b < galois->order; ++b) {
            // Add completeness.
            uint32_t sum = galois_add(a, b);
            assert(galois_in(galois, sum));
            assert(a == galois_add(b, sum));
            assert(b == galois_add(a, sum));

            // Multiply completeness.
            uint32_t prod = galois_mul(galois, a, b);
            assert(galois_in(galois, prod));
            if (a != 0 && b != 0) {
                assert((galois_index(galois, a) + galois_index(galois, b)) % (galois_order(galois)-1) == galois_index(galois, prod));
            } else {
                assert(prod == 0);
            }

            // Divide
            if (b != 0) {
                uint32_t quotient = galois_div(galois, a, b);
                assert(galois_mul(galois, quotient, b) == a);
            }

        }
        // Reciprocal
        if (a != 0) {
            uint32_t reciprocal = galois_inv(galois, a);
            assert(reciprocal == galois_div(galois, 1, a));
            assert(1 == galois_mul(galois, a, reciprocal));
        }
    }

    printf("Succeed!\n");
//    SBUILDER(builder, 1024);
//    for (int i = 0; i < galois->order; ++i) {
//        sbuilder_reset(&builder);
//        sbuilder_format(&builder, "%3d. exp = ", i);
//        sbuilder_binary(&builder, galois->exps[i], galois->degree);
//        sbuilder_str(&builder, ", log = ");
//        sbuilder_binary(&builder, galois->logs[i], galois->degree);
//        printf("%s\n", builder.buf);
//    }
}

void test_poly(galois_t * galois) {
    SBUILDER(builder, 1024);

    uint32_t T = 6;
    uint32_t gen_poly[galois->order - 1];
    galois_poly_zero(gen_poly, T);
    gen_poly[0] = galois_element(galois, 0);
    for (uint32_t i = 1; i <= T; ++i) {
        galois_poly_shift_add_scale(galois, gen_poly, i - 1, galois_element(galois, i));
        sbuilder_reset(&builder);
        sbuilder_galois_poly(&builder, galois, gen_poly, T);
        printf("rscode.gen_poly = %s\n", builder.buf);
    }

//    for (int i = 0; i < galois->order; ++i) {
//        sbuilder_reset(&builder);
//        sbuilder_format(&builder, "%3d. exp = ", i);
//        sbuilder_binary(&builder, galois->exps[i], galois->degree);
//        sbuilder_str(&builder, ", log = ");
//        sbuilder_binary(&builder, galois->logs[i], galois->degree);
//        printf("%s\n", builder.buf);
//    }
}

int main() {
    GALOIS16(galois16);
    test(&galois16);
    test_poly(&galois16);

    GALOIS256(galois256);
    test(&galois256);
    return 0;
}
