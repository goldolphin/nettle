#include <stdio.h>
#include <utils/utils.h>
#include <utils/sbuilder.h>
#include <tunnel/nt_dup_fec.h>
#include <tunnel/nt_rs_fec.h>
#include <tunnel/nt_perf.h>
#include <echo/udp_cs.h>
#include <echo/packet.h>

#define MAX_ADDRS 32
#define MAX_SEQ_NUM 256
#define MAX_DUP_NUM 16
#define DECODE_BUFFER_SIZE 10000

int main(int argc, char **argv) {
    int argi = 1;
    char * type = argv[argi ++];

    // Init perf.
    perf_t perf;
    perf_init(&perf, nt_counter_names, nt_counter_num);

    SBUILDER(builder, 4096);
    error_t err = 0;
    if (strcmp("-c", type) == 0) {
        nt_path_t paths[MAX_ADDRS];
        int path_num = MAX_ADDRS;
        parse_paths(paths, &path_num, argv[argi ++]);
        int count = atoi(argv[argi ++]);
        int qps = atoi(argv[argi ++]);
        char * strategy_str = argv[argi ++];
        nt_distributor_t distributor;
        nt_fec_t * p_fec;
        idgen_t idgen;
        idgen_init(&idgen, 0);
        if (strcmp("dup", strategy_str) == 0) {
            log_info("Strategy=%s", "DUP");
            nt_dup_fec_t fec;
            nt_dup_fec_init(&fec, (uint8_t) path_num, 0, 0, &idgen, &perf);
            int seq2addr[path_num];
            for (int i = 0; i < path_num; ++i) {
                seq2addr[i] = i;
            }
            if (!nt_distributor_init(&distributor, paths, path_num, seq2addr, path_num, 1)) {
                log_error("Error initializing strategy.");
                return 1;
            }
            sbuilder_reset(&builder);
            sbuilder_distributor(&builder, &distributor);
            log_info("distributor = %s", builder.buf);
            p_fec = (nt_fec_t *) &fec;
        } else if (strcmp("rs", strategy_str) == 0) {
            uint8_t N = (uint8_t) atoi(argv[argi ++]);
            uint8_t K = (uint8_t) atoi(argv[argi ++]);
            int seq2addr[MAX_SEQ_NUM*MAX_DUP_NUM];
            parse_seq2addr(seq2addr, MAX_SEQ_NUM, MAX_DUP_NUM, argv[argi ++]);
            log_info("Strategy=%s, N=%u, K=%u", "RS", N, K);
            nt_rs_fec_t fec;
            nt_rs_fec_init(&fec, N, K, sizeof(packet_t), DECODE_BUFFER_SIZE, 2000, &idgen, &perf);
            if (!nt_distributor_init(&distributor, paths, path_num, seq2addr, MAX_SEQ_NUM, MAX_DUP_NUM)) {
                log_error("Error initializing strategy.");
                return 1;
            }
            sbuilder_reset(&builder);
            sbuilder_distributor(&builder, &distributor);
            log_info("distributor = %s", builder.buf);
            p_fec = (nt_fec_t *) &fec;
        } else {
            ensure(false);
        }
        address_t tun_addr;
        address_t * p_tun_addr = NULL;
        if (argi < argc) {
            parse_address(&tun_addr, argv[argi++]);
            p_tun_addr = &tun_addr;
        }
        err = udp_client(p_fec, &distributor, count, qps, p_tun_addr, NT_ENCRYPT_TYPE_KC);
    } else if(strcmp("-s", type) == 0) {
        address_t address;
        parse_address(&address, argv[argi ++]);
        err = udp_server(&address);
    } else {
        log_error("Invalid arguments!");
        return 1;
    }
    sbuilder_reset(&builder);
    sbuilder_perf(&builder, &perf);
    log_info("Perf Counters = %s", builder.buf);
    return err;
}
