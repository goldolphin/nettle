#include <stdio.h>
#include <errno.h>
#include <utils/sbuilder.h>
#include <utils/config.h>
#include <tunnel/nt_distributor.h>
#include <tunnel/nt_dup_fec.h>
#include <echo/udp_cs.h>
#include <tunnel/nt_perf.h>

/**
 * @author caofuxiang
 *         2015-08-07 10:36:36.
 */
#define MAX_PATH_NUM 32

error_t ping(nt_path_t * paths, int path_num, int count, int qps, address_t * tun_addr, idgen_t * idgen, perf_t * perf) {
    nt_dup_fec_t fec;
    nt_dup_fec_init(&fec, 1, 0, 0, idgen, perf);
    int seq2addr[path_num];
    for (int i = 0; i < path_num; ++i) {
        seq2addr[i] = i;
    }
    nt_distributor_t distributor;
    ensure(nt_distributor_init(&distributor, paths, path_num, seq2addr, path_num, 1));

    return udp_client((nt_fec_t *) &fec, &distributor, count, qps, tun_addr, NT_ENCRYPT_TYPE_KC);
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Invalid arguments.\nUsage: %s <config-path>\n", argv[0]);
        return EINVAL;
    }
    const char *config_path = argv[1];
    SBUILDER(builder, 4096);

    // Read configuration.
    config_t config;
    config_init(&config);
    ensure(config_load_from_file(&config, config_path));

    if (config_get(&config, "ping_qps") == NULL) {
        return 1;
    }
    int qps = atoi(config_get(&config, "ping_qps"));
    int count = atoi(config_get(&config, "ping_count"));

    address_t api_local;
    parse_address(&api_local, config_get(&config, "api_local"));

    int path_num = MAX_PATH_NUM;
    nt_path_t paths[MAX_PATH_NUM];
    parse_paths(paths, &path_num, config_get(&config, "ping_paths"));

    address_t tunnel_addr;
    address_from(&tunnel_addr, "127.0.0.1", NULL, 0);
    address_set_port(&tunnel_addr, address_get_port(&api_local));

    idgen_t idgen;
    idgen_init(&idgen, 0);
    perf_t perf;
    perf_init(&perf, nt_counter_names, nt_counter_num);
    return ping(paths, 1, count, qps, &tunnel_addr, &idgen, &perf);
}
