/**
 * @author caofuxiang
 *         2015-07-01 20:24:24.
 */

#include <utils/config.h>
#include <utils/logger.h>
#include <unistd.h>
#include <utils/utils.h>
#include "nt_fec_processor.h"
#include "nt_rs_fec.h"
#include "nt_listener.h"
#include "nt_reporter.h"
#include "nt_dup_fec.h"
#include "nt_echo_listener.h"

#define MAX_PATH_NUM 32
#define MAX_SEQ_NUM 256
#define MAX_DUP_NUM 16

static void * start_processor(void * processor) {
    return NULL + nt_fec_processor_run(processor);
}

static void * start_listener(void * listener) {
    return NULL + nt_listener_run(listener);
}

static void * start_reporter(void * reporter) {
    return NULL + nt_reporter_run(reporter);
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Invalid arguments.\nUsage: %s <config-path>\n", argv[0]);
        return EINVAL;
    }
    const char * config_path = argv[1];
    SBUILDER(builder, 4096);

    // Init signal handlers.
    set_signal_backtrace(SIGSEGV);
    set_signal_backtrace(SIGABRT);

    // Read configuration.
    config_t config;
    config_init(&config);
    ensure(config_load_from_file(&config, config_path));

    /// Misc
    log_set_priority(log_parse_priority(config_get_default(&config, "log_priority", "ALL")));

    /// FEC
    uint8_t N = (uint8_t) atoi(config_get(&config, "N"));
    uint8_t K = (uint8_t) atoi(config_get(&config, "K"));
    int max_fec_data_size = atoi(config_get(&config, "max_fec_data_size"));
    int decode_buffer_num = atoi(config_get(&config, "decode_buffer_num"));
    long decode_timeout = atoi(config_get_default(&config, "decode_timeout", "2000"));

    /// Network
    size_t max_udp_buffer_size = (size_t) atoi(config_get(&config, "max_udp_buffer_size"));

    address_t api_local;
    parse_address(&api_local, config_get(&config, "api_local"));

    address_t tun_local;
    parse_address(&tun_local, config_get(&config, "tun_local"));

    address_t reporter_local;
    parse_address(&reporter_local, config_get(&config, "reporter_local"));

    address_t echo_local;
    bool enable_echo = false;
    const char * echo_local_str = config_get(&config, "echo_local");
    if (echo_local_str != NULL) {
        enable_echo = true;
        parse_address(&echo_local, echo_local_str);
    }

    bool enable_fec = str2bool(config_get_default(&config, "enable_fec", "true"));
    bool enable_direct = str2bool(config_get_default(&config, "enable_direct", "false"));

    int path_num = MAX_PATH_NUM;
    nt_path_t paths[MAX_PATH_NUM];
    int seq2addr[MAX_SEQ_NUM*MAX_DUP_NUM];
    if (enable_fec) {
        parse_paths(paths, &path_num, config_get(&config, "paths"));
        parse_seq2addr(seq2addr, MAX_SEQ_NUM, MAX_DUP_NUM, config_get(&config, "seq2addr"));
    }

    nt_path_t direct_path;
    if (enable_direct) {
        parse_path(&direct_path, config_get(&config, "direct_path"));
    }

    /// Encryption
    const char * ka_host = NULL;
    int ka_port = 0;
    const char * ka_sid = NULL;
    bool enable_encrypt = str2bool(config_get_default(&config, "enable_encrypt", "true"));
    if (enable_encrypt) {
        ka_host = config_get(&config, "ka_host");
        ka_port = atoi(config_get(&config, "ka_port"));
        ka_sid = config_get(&config, "ka_sid");
    }

    /// Performance
    int processor_num = atoi(config_get(&config, "processor_num"));
    size_t mq_capacity = (size_t) atoi(config_get(&config, "mq_capacity"));

    // Init perf.
    perf_t perf;
    perf_init(&perf, nt_counter_names, nt_counter_num);

    // Init UDPs.
    log_info("Initialize UDPs.");
    udp_t api_udp;
    udp_init(&api_udp, &api_local, true);
    udp_t tun_udp;
    udp_init(&tun_udp, &tun_local, true);

    // Init FEC.
    nt_fec_processor_t * processors = NULL;
    nt_fec_processor_t processors0[processor_num];
    idgen_t idgen;
    idgen_init(&idgen, 0);
    nt_fec_t * fecs[processor_num];
    ka_encryptor_t encryptors[processor_num];
    pthread_t processor_tids[processor_num];
    nt_distributor_t distributor;
    if (enable_fec) {
        /// Init distributor.
        nt_distributor_init(&distributor, paths, path_num, seq2addr, MAX_SEQ_NUM, MAX_DUP_NUM);
        sbuilder_reset(&builder);
        sbuilder_distributor(&builder, &distributor);
        log_info("distributor = %s", builder.buf);

        /// Init processors.
        processors = processors0;
        log_info("FEC parameters: N=%u, K=%u, max_fec_data_size=%d, decode_buffer_num=%d, decode_timeout=%ld",
                 N, K, max_fec_data_size, decode_buffer_num, decode_timeout);
        log_info("Initialize processors: processor_num=%d, mq_capacity=%lu", processor_num, mq_capacity);
        for (int i = 0; i < processor_num; ++i) {
            if (K == 1) {
                fecs[i] = (nt_fec_t *) new_data(nt_dup_fec_t);
                nt_dup_fec_init((nt_dup_fec_t *) fecs[i], N, decode_buffer_num / processor_num, decode_timeout, &idgen, &perf);
            } else {
                fecs[i] = (nt_fec_t *) new_data(nt_rs_fec_t);
                nt_rs_fec_init((nt_rs_fec_t *) fecs[i], N, K, max_fec_data_size, decode_buffer_num / processor_num, decode_timeout, &idgen, &perf);
            }
            ka_encryptor_t * encryptor = NULL;
//            if (enable_encrypt) {
//                encryptor = &encryptors[i];
//                ka_encryptor_init(encryptor, ka_host, ka_port, ka_sid, KA_COMPRESSION_NONE);
//                ensure(ka_encryptor_start(encryptor));
//            }
            ensure(nt_fec_processor_init(&processors[i], mq_capacity, &distributor, &api_udp, &tun_udp, fecs[i],
                                  encryptor, &perf) == 0);
            ensure(pthread_create(&processor_tids[i], NULL, start_processor, &processors[i]) == 0);
        }
    }

    // Init pass-through forwarder.
    nt_forwarder_t * forwarder = NULL;
    nt_forwarder_t forwarder0;
    if (enable_direct) {
        sbuilder_reset(&builder);
        sbuilder_path(&builder, &direct_path);
        log_info("Initialize direct forwarding: path=%s", builder.buf);
        forwarder = &forwarder0;
        nt_forwarder_init(forwarder, &direct_path, &api_udp, &tun_udp);
    }

    // Init tunnel listener.
    nt_fec_listener_context_t tun_listener_context;
    nt_fec_listener_context_init(&tun_listener_context, processors, processor_num, forwarder, &perf);

    sbuilder_reset(&builder);
    sbuilder_address(&builder, &tun_local);
    log_info("Initialize tun listener: max_udp_buffer_size=%lu, mq_capacity=%lu, local_address=%s", max_udp_buffer_size, mq_capacity, builder.buf);
    nt_listener_t tun_listener;
    ensure(nt_listener_init(&tun_listener, &tun_udp, mq_capacity, max_udp_buffer_size, &tun_listener_context, nt_tun_handle_read) == 0);
    pthread_t tun_listener_tid;
    ensure(pthread_create(&tun_listener_tid, NULL, start_listener, &tun_listener) == 0);

    // Init API listener
    nt_fec_listener_context_t api_listener_context;
    nt_fec_listener_context_init(&api_listener_context, processors, processor_num, forwarder, &perf);

    sbuilder_reset(&builder);
    sbuilder_address(&builder, &api_local);
    log_info("Initialize api listener: max_udp_buffer_size=%lu, mq_capacity=%lu, local_address=%s", max_udp_buffer_size, mq_capacity, builder.buf);
    nt_listener_t api_listener;
    ensure(nt_listener_init(&api_listener, &api_udp, mq_capacity, max_udp_buffer_size, &api_listener_context, nt_api_handle_read) == 0);
    pthread_t api_listener_tid;
    ensure(pthread_create(&api_listener_tid, NULL, start_listener, &api_listener) == 0);

    // Start reporter
    sbuilder_reset(&builder);
    sbuilder_address(&builder, &reporter_local);
    log_info("Initialize reporter: local_address=%s", builder.buf);
    nt_reporter_t reporter;
    ensure(nt_reporter_init(&reporter, &reporter_local, &perf) == 0);
    pthread_t reporter_tid;
    ensure(pthread_create(&reporter_tid, NULL, start_reporter, &reporter) == 0);

    // Start echo listener
    if (enable_echo) {
        sbuilder_reset(&builder);
        sbuilder_address(&builder, &echo_local);
        log_info("Initialize echo listener: local_address=%s", builder.buf);
        udp_t echo_udp;
        udp_init(&echo_udp, &echo_local, true);
        nt_echo_listener_t echo_listener;
        ensure(nt_echo_listener_init(&echo_listener, &echo_udp, decode_buffer_num / processor_num, decode_timeout) == 0);
        pthread_t echo_listener_tid;
        ensure(pthread_create(&echo_listener_tid, NULL, start_listener, &echo_listener.listener) == 0);
    }

    // Join.
    log_info("Service is running ...");
    void * status;
    int ret = pthread_join(api_listener_tid, &status);
    error_t err = (error_t) (int64_t)status;
    if (err != 0) {
        log_warn("Thread stops for: %s (%d)", strerror(err), err);
    }
    if (ret != 0) {
        log_warn("Error on joining: %s (%d)", strerror(ret), err);
    }
    return ret;
}
