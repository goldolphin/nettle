/**
 * @author caofuxiang
 *         2015-06-25 14:52:52.
 */

#include <assert.h>

#include <utils/sbuilder.h>
#include <rs/rs.h>
#include <utils/utils.h>
#include <echo/packet.h>

typedef struct {
    int degree;
    uint8_t payload[RS_MAX_ENCODED_SIZE(2000)];
    int size;
} encoded_t;

static void perf(int N, int K, int round, bool to_decode, bool to_compare) {
    packet_t * packets = malloc(round * sizeof(packet_t));
    for (int r = 0; r < round; ++r) {
        packet_init(&packets[r], r, r);
        packets[r].seq = (uint64_t) r;
    }
    int T = N-K;
    int size = sizeof(packet_t);

    rs_t rs;
    rs_init(&rs, N, K, size);

    rs_ec_t ec;
    rs_init_ec(&rs, &ec);

    rs_dc_t dc;
    rs_init_dc(&rs, &dc);

    uint32_t erasure_num = (uint32_t) (T < K ? T : K);
    encoded_t encoded[T];
    packet_t decoded[erasure_num];

    long begin = current_millis();
    for (int r = 0; r < round; ++r) {
        // Encode
        int degree;
        int n = rs_encode(&rs, &ec, (uint8_t *) &packets[r], size, &degree);

        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                encoded[i].size = sizeof encoded[i].payload;
                assert(rs_encode_result(&rs, &ec, i, encoded[i].payload, &encoded[i].size, &encoded[i].degree) >= 0);
            }

            // Decode, we use the last N-erasure_num packets to decode the first erasure_num packets.
            if (to_decode) {
                int begin_packet = r / K * K;
                for (int j = erasure_num; j < K; ++j) {
                    if (rs_is_dc_complete(&rs, &dc)) continue;
                    bool is_raw;
                    int nd = rs_decode(&rs, &dc, (uint8_t *) &packets[begin_packet + j], size, N - 1 - j, &is_raw);
                    assert(is_raw);
                    assert(nd == 0);
                }

                for (int j = 0; j < T; ++j) {
                    if (rs_is_dc_complete(&rs, &dc)) continue;
                    bool is_raw;
                    int nd = rs_decode(&rs, &dc, encoded[j].payload, encoded[j].size, encoded[j].degree, &is_raw);
                    assert(!is_raw);
                    for (int i = 0; i < nd; ++i) {
                        int decoded_size = size;
                        int decoded_degree;
                        assert(rs_decode_result(&rs, &dc, i, (uint8_t *) &decoded[i], &decoded_size, &decoded_degree) >= 0);
                        assert(decoded_size == size);
                    }
                }

                // Compare
                if (to_compare) {
                    for (uint32_t j = 0; j < erasure_num; ++j) {
                        size_t id = begin_packet + j;
                        assert(decoded[j].id == id);
                        assert(memcmp(&decoded[j], &packets[id], (size_t)size) == 0);
                    }
                }
                rs_reset_dc(&rs, &dc);
            }
            rs_reset_ec(&rs, &ec);
        }
    }
    long elapse = current_millis()-begin;
    double throughput = (double)size * round * 8/elapse/1000;
    char * decode_str = to_decode ? "true" : "false";
    char * compare_str = to_decode && to_compare ? "true" : "false";
    double elapse_per_packet = (double)elapse/round;
    double elapse_per_erasure = (double)elapse/(round/K*erasure_num);
    printf("N=%d, K=%d, round=%d, decode=%s, compare=%s, packet_size=%d, elapse=%ldms, bps=%.4fMbps, elapse_per_packet=%.2fms, elapse_per_erasure=%.2fms\n",
           N, K, round, decode_str, compare_str, size, elapse, throughput, elapse_per_packet, elapse_per_erasure);

    rs_destroy_ec(&ec);
    rs_destroy_dc(&dc);
}

int main(int argc, char **argv) {
    int N = 10;
    int K = 5;
    int round = 2000;
    bool to_decode = true;
    bool to_compare = true;

    if (argc > 1) N = (int) atol(argv[1]);
    if (argc > 2) K = (int) atol(argv[2]);
    if (argc > 3) round = (int) atol(argv[3]);
    if (argc > 4) to_decode = strcmp("t", argv[4]) == 0 ? true : false;
    if (argc > 5) to_compare = strcmp("t", argv[5]) == 0 ? true : false;
    perf(N, K, round, to_decode, to_compare);
    return 0;
}
