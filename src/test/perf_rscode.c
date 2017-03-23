/**
 * @author caofuxiang
 *         2015-06-16 15:35:35.
 */

#include <assert.h>

#include <utils/sbuilder.h>
#include <rs/galois.h>
#include <rs/rscode.h>
#include <echo/packet.h>

void perf(size_t N, size_t K, int round, bool to_decode, bool to_compare) {
    GALOIS256(galois);
    size_t T = N-K;
    uint32_t rs_K = (uint32_t) (galois_order(&galois)-1-T);
    RSCODE(rscode, rs_K, &galois);

    packet_t * packets = malloc(round * sizeof(packet_t));
    for (int r = 0; r < round; ++r) {
        packet_init(&packets[r], r, r);
        packets[r].seq = (uint64_t) r;
    }

    size_t size = sizeof(packet_t);

    rs_buffer_t send_buffer;
    rs_buffer_init(&send_buffer, T);

    rs_buffer_t recv_buffer;
    rs_buffer_init(&recv_buffer, T);

    uint32_t erasure_num = (uint32_t) (T < K ? T : K);
    uint32_t erasure_degrees[erasure_num];
    for (int k = 0; k < erasure_num; ++k) {
        erasure_degrees[k] = (uint32_t) (N-1-k);
    }
    packet_t redundant_packets[T];

    long begin = current_millis();
    for (int r = 0; r < round; ++r) {
        // Encode
        uint8_t * b = (uint8_t *) &packets[r];
        for (size_t i = 0; i < size; ++i) {
            rscode_encode(&rscode, b[i], send_buffer.data + i*T);
        }

        if (r % K == K-1) {
            for (size_t j = 0; j < T; ++j) {
                build_packet(&redundant_packets[j], send_buffer.data, T, j);
            }

            // Decode, we use the last N-erasure_num packets to decode the first erasure_num packets.
            if (to_decode) {
                uint32_t degree = (uint32_t) (N - 1 - erasure_num);
                for (int j = (int) (K - 1 - erasure_num); j >= 0; --j) {
                    uint8_t *c = (uint8_t *) &packets[r - j];
                    for (size_t i = 0; i < size; ++i) {
                        rscode_decode_syndromes(&rscode, c[i], degree, recv_buffer.data + i * T);
                    }
                    degree--;
                }

                for (int j = 0; j < T; ++j) {
                    uint8_t * c = (uint8_t *) &redundant_packets[j];
                    for (size_t i = 0; i < size; ++i) {
                        rscode_decode_syndromes(&rscode, c[i], degree, recv_buffer.data + i * T);
                    }
                    degree--;
                }

                // Compare
                if (to_compare) {
                    int begin_packet = (int) (r / K * K);
                    for (uint32_t j = 0; j < erasure_num; ++j) {
                        packet_t recv_packet;
                        for (uint32_t i = 0; i < sizeof(packet_t); ++i) {
                            ((uint8_t *) &recv_packet)[i] = (uint8_t) rscode_decode_erasure(&rscode, recv_buffer.data + i*T, erasure_degrees, erasure_num, j);
                        }
                        size_t seq = begin_packet + N - 1 - erasure_degrees[j];
                        assert(recv_packet.seq == seq);
                        assert(memcmp(&recv_packet, &packets[seq], size) == 0);
                    }
                }
                rs_buffer_reset(&recv_buffer, T);
            }
            rs_buffer_reset(&send_buffer, T);
        }
    }
    long elapse = current_millis()-begin;
    double throughput = (double)size * round * 8/elapse/1000;
    char * decode_str = to_decode ? "true" : "false";
    char * compare_str = to_decode && to_compare ? "true" : "false";
    double elapse_per_packet = (double)elapse/round;
    double elapse_per_erasure = (double)elapse/(round/K*erasure_num);
    printf("N=%lu, K=%lu, round=%d, decode=%s, compare=%s, packet_size=%lu, elapse=%ldms, bps=%.4fMbps, elapse_per_packet=%.2fms, elapse_per_erasure=%.2fms\n",
           N, K, round, decode_str, compare_str, size, elapse, throughput, elapse_per_packet, elapse_per_erasure);
}

int main(int argc, char **argv) {
    size_t N = 10;
    size_t K = 5;
    int round = 2000;
    bool to_decode = true;
    bool to_compare = true;

    if (argc > 1) N = (size_t) atol(argv[1]);
    if (argc > 2) K = (size_t) atol(argv[2]);
    if (argc > 3) round = (int) atol(argv[3]);
    if (argc > 4) to_decode = strcmp("t", argv[4]) == 0 ? true : false;
    if (argc > 5) to_compare = strcmp("t", argv[5]) == 0 ? true : false;
    perf(N, K, round, to_decode, to_compare);
    return 0;
}
