/**
 * @author caofuxiang
 *         2015-06-25 16:42:42.
 */
#include <assert.h>

#include <utils/sbuilder.h>
#include <rs/rs.h>

typedef struct {
    int degree;
    uint8_t payload[RS_MAX_ENCODED_SIZE(1)];
    int size;
} encoded_t;

void print_array(char * name, uint32_t * array, int len) {
    SBUILDER(builder, 1024);
    for (size_t i = 0; i < len; ++i) {
        if (i > 0) {
            sbuilder_char(&builder, ' ');
        }
        sbuilder_format(&builder, "%d", array[i]);
    }
    printf("%s=%s\n", name, builder.buf);
}

void erase(char * codeword, encoded_t * encoded, int len, uint32_t * erasure_degrees, uint32_t erasures_num) {
    for (int i = 0; i < erasures_num; ++i) {
        int n = len-1-erasure_degrees[i];
        codeword[n] = 0;
        encoded[n].degree = -1;
    }
}

bool sbuilder_code(sbuilder_t * builder, char * code, int len) {
    for (size_t i = 0; i < len; ++i) {
        if (i > 0) {
            sbuilder_char(builder, ' ');
        }
        sbuilder_format(builder, "%02X", (uint8_t)code[i]);
    }
    return true;
}

char * code2str(sbuilder_t * builder, char * code, int len) {
    sbuilder_reset(builder);
    sbuilder_code(builder, code, len);
    return builder->buf;
}

bool codecmp(char * code1, char * code2, int len) {
    for (int i = 0; i < len; ++i) {
        if (code1[i] != code2[i]) return false;
    }
    return true;
}

void test_rs(rs_t * rs, char * message, int len, uint32_t * erasure_degrees, uint32_t erasure_num) {
    printf("\n****** Test ******\n");
    SBUILDER(builder, 1024);

    rs_ec_t ec;
    rs_init_ec(rs, &ec);

    rs_dc_t dc;
    rs_init_dc(rs, &dc);

    sbuilder_binary(&builder, rs->rscode.galois->gen_poly, rs->rscode.galois->degree+1);
    printf("galois.gen_poly = %s\n", builder.buf);

    sbuilder_reset(&builder);
    sbuilder_binary(&builder, galois_inv(rs->rscode.galois, 1), rs->rscode.galois->degree);
    printf("inv(1) = %s\n", builder.buf);

    uint32_t T = rscode_checks_num(&rs->rscode);

    sbuilder_reset(&builder);
    sbuilder_galois_poly(&builder, rs->rscode.galois, rs->rscode.gen_poly, T);
    printf("rscode.gen_poly = %s\n", builder.buf);

    // encode
    printf("message=%s\n", code2str(&builder, message, len));

    char codeword[len+T];
    encoded_t encoded[len+T];
    for (int i = 0, k = 0; i < len; ++i) {
        int degree;
        int n = rs_encode(rs, &ec, (uint8_t *) &message[i], 1, &degree);
        assert(n >= 0);
        encoded[k].degree = degree;
        memcpy(encoded[k].payload, (uint8_t *) &message[i], 1);
        encoded[k].size = 1;
        ++k;

        for (int j = 0; j < n; ++j) {
            encoded[k].size = sizeof encoded[k].payload;
            assert(rs_encode_result(rs, &ec, j, encoded[k].payload, &encoded[k].size, &encoded[k].degree) >= 0);
            ++k;
        }
    }
    for (int i = 0; i < sizeof codeword; ++i) {
        int n = i < rs->K ? 0 : RS_HEADER_SIZE;
        codeword[i] = (char) (n < encoded[i].size ? encoded[i].payload[n] : 0);
    }
    printf("codeword=%s\n", code2str(&builder, codeword, sizeof codeword));

    erase(codeword, encoded, sizeof codeword, erasure_degrees, erasure_num);
    printf("erased codeword=%s\n", code2str(&builder, codeword, sizeof codeword));

    char decoded_message[len];
    for (int i = 0; i < sizeof codeword; ++i) {
        if (encoded[i].degree >= 0) { // not erased.
            bool is_raw;
            int n = rs_decode(rs, &dc, encoded[i].payload, encoded[i].size, encoded[i].degree, &is_raw);
            assert(n >= 0);
            if (is_raw) {
                decoded_message[i] = encoded[i].payload[0];
            }
            for (int j = 0; j < n; ++j) {
                uint8_t v[4];
                int size = 4;
                int degree;
                assert(rs_decode_result(rs, &dc, j, v, &size, &degree) >= 0);
                assert(size == 1);
                decoded_message[rs->N-1-degree] = v[0];
            }
        }
    }
    printf("decoded message=%s\n", code2str(&builder, decoded_message, len));

    assert(codecmp(message, decoded_message, len));

//    for (int i = 0; i < rs->rscode.galois->order-1; ++i) {
//        sbuilder_reset(&builder);
//        sbuilder_format(&builder, "%3d. exp = ", i);
//        sbuilder_binary(&builder, rs->rscode.galois->exps[i], rs->rscode.galois->degree);
//        sbuilder_str(&builder, ", log = ");
//        sbuilder_binary(&builder, rs->rscode.galois->logs[i], rs->rscode.galois->degree);
//        printf("%s\n", builder.buf);
//    }
    rs_destroy_ec(&ec);
    rs_destroy_dc(&dc);
}

int main() {
    char message9[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t erasure_degrees6[6] = {14, 10, 6, 7, 1, 0};
    rs_t rs_15_9;
    rs_init(&rs_15_9, 15, 9, 1);
    test_rs(&rs_15_9, message9, 9, erasure_degrees6, 6);

    rs_t rs_30_23;
    rs_init(&rs_30_23, 30, 23, 1);
    char message23[23] = "0123456789ABCDEFGHIJKL";
    uint32_t erasure_degrees7[7] = {23, 18, 10, 16, 17, 11, 20};
    test_rs(&rs_30_23, message23, 23, erasure_degrees7, 7);

    rs_t rs_3_2;
    rs_init(&rs_3_2, 3, 2, 1);
    char message2[2] = {1, 2};
    uint32_t erasure_degrees1[1] = {2};
    test_rs(&rs_3_2, message2, 2, erasure_degrees1, 1);

    rs_t rs_3_1;
    rs_init(&rs_3_1, 3, 1, 1);
    char message1[1] = {1};
    uint32_t erasure_degrees2[2] = {2, 1};
    test_rs(&rs_3_1, message1, 1, erasure_degrees2, 2);

    printf("\nAll Succeed!\n");
    return 0;
}
