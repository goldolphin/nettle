/**
 * @author caofuxiang
 *         2015-06-10 10:28:28.
 */

#include <assert.h>

#include <utils/sbuilder.h>
#include <rs/galois.h>
#include <rs/rscode.h>

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

void encode(rscode_t * rscode, char * message, int message_len, char * codeword) {
    uint32_t num = rscode_checks_num(rscode);
    uint32_t checks[num];
    rscode_prepare(rscode, checks);
    for (int i = 0; i < message_len; ++i) {
        codeword[i] = message[i];
        rscode_encode(rscode, (uint8_t) message[i], checks);
    }
    print_array("checks", checks, num);
    for (int i = 0; i < num; ++i) {
        codeword[i+ message_len] = (char) checks[i];
    }
}

void decode(rscode_t * rscode, char * message, int message_len, char * codeword, uint32_t * erasure_degrees, uint32_t erasure_num) {
    uint32_t num = rscode_checks_num(rscode);
    int len = message_len + num;
    uint32_t syndromes[num];
    rscode_prepare(rscode, syndromes);
    for (int i = 0; i < len; ++i) {
        rscode_decode_syndromes(rscode, (uint8_t) codeword[i], (uint32_t) (len-1-i), syndromes);
    }
    print_array("syndromes", syndromes, num);
    printf("rscode_check_syndromes: %s\n", rscode_check_syndromes(rscode, syndromes) ? "true" : "false");

    uint32_t erasure_values[erasure_num];
    rscode_decode_erasures(rscode, syndromes, erasure_degrees, erasure_num, erasure_values);
    for (int i = 0; i < message_len; ++i) {
        message[i] = codeword[i];
    }
    for (int i = 0; i < erasure_num; ++i) {
        int n = len-1-erasure_degrees[i];
        if (n < message_len) {
            message[n] = (char) erasure_values[i];
        }
    }
}

void erase(char * codeword, int len, uint32_t * erasure_degrees, uint32_t erasures_num) {
    for (int i = 0; i < erasures_num; ++i) {
        int n = len-1-erasure_degrees[i];
        codeword[n] = 0;
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

void test_rs(rscode_t * rscode, char * message, int len, uint32_t * erasure_degrees, uint32_t erasure_num) {
    printf("\n****** Test ******\n");
    SBUILDER(builder, 1024);

    sbuilder_binary(&builder, rscode->galois->gen_poly, rscode->galois->degree+1);
    printf("galois.gen_poly = %s\n", builder.buf);

    sbuilder_reset(&builder);
    sbuilder_binary(&builder, galois_inv(rscode->galois, 1), rscode->galois->degree);
    printf("inv(1) = %s\n", builder.buf);

    uint32_t T = rscode_checks_num(rscode);

    sbuilder_reset(&builder);
    sbuilder_galois_poly(&builder, rscode->galois, rscode->gen_poly, T);
    printf("rscode.gen_poly = %s\n", builder.buf);

    return;
    // encode
    char codeword[len+T];
    printf("message=%s\n", code2str(&builder, message, len));

    encode(rscode, message, len, codeword);
    printf("codeword=%s\n", code2str(&builder, codeword, sizeof codeword));

    erase(codeword, sizeof codeword, erasure_degrees, erasure_num);
    printf("erased codeword=%s\n", code2str(&builder, codeword, sizeof codeword));

    char decoded_message[len];
    decode(rscode, decoded_message, sizeof decoded_message, codeword, erasure_degrees, erasure_num);
    printf("decoded message=%s\n", code2str(&builder, decoded_message, len));

//    for (int i = 0; i < rscode->galois->order; ++i) {
//        sbuilder_reset(&builder);
//        sbuilder_format(&builder, "%3d. exp = ", i);
//        sbuilder_binary(&builder, rscode->galois->exps[i], rscode->galois->degree);
//        sbuilder_str(&builder, ", log = ");
//        sbuilder_binary(&builder, rscode->galois->logs[i], rscode->galois->degree);
//        printf("%s\n", builder.buf);
//    }

    assert(codecmp(message, decoded_message, len));
}

int main() {
    char message9[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t erasure_degrees6[6] = {14, 10, 6, 7, 1, 0};
    GALOIS16(galois16);
    RSCODE(rscode_15_9, 9, &galois16);
    test_rs(&rscode_15_9, message9, 9, erasure_degrees6, 6);

    GALOIS256(galois256);
    RSCODE(rscode_255_248, 248, &galois256);
    test_rs(&rscode_255_248, message9, 9, erasure_degrees6, 6);

    char message23[23] = "0123456789ABCDEFGHIJKL";
    uint32_t erasure_degrees7[7] = {23, 18, 10, 16, 17, 11, 20};
    test_rs(&rscode_255_248, message23, 23, erasure_degrees7, 7);

    RSCODE(rscode_255_254, 254, &galois256);
    char message2[2] = {1, 2};
    uint32_t erasure_degrees1[1] = {2};
    test_rs(&rscode_255_254, message2, 2, erasure_degrees1, 1);

    printf("\nAll Succeed!\n");
    return 0;
}
