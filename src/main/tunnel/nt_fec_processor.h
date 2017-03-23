/**
 * @author caofuxiang
 *         2015-07-01 10:17:17.
 */

#ifndef NETTLE_NT_FEC_PROCESSOR_H
#define NETTLE_NT_FEC_PROCESSOR_H

#include <dispatcher/perf.h>
#include <dispatcher/worker.h>
#include <dispatcher/udp.h>
#include <keycenter/ka_encryptor.h>
#include "nt_proto.h"
#include "nt_distributor.h"
#include "nt_fec.h"

typedef struct {
    dispatcher_t dispatcher;
    worker_t encoder;
    worker_t decoder;
    nt_distributor_t * distributor;
    udp_t * api_udp;
    udp_t * tun_udp;
    nt_fec_t * fec;
    ka_encryptor_t * encryptor;
    perf_t * perf;
} nt_fec_processor_t;

error_t nt_fec_processor_init(nt_fec_processor_t *processor, size_t mq_capacity,
                              nt_distributor_t *distributor, udp_t * api_udp, udp_t * tun_udp,
                              nt_fec_t *fec, ka_encryptor_t * encryptor, perf_t * perf);

error_t nt_fec_processor_destroy(nt_fec_processor_t *processor);

error_t nt_fec_processor_run(nt_fec_processor_t *processor);

bool nt_fec_processor_encode(nt_fec_processor_t *processor, void * message, worker_free_t free);

bool nt_fec_processor_decode(nt_fec_processor_t *processor, void * message, worker_free_t free);

#endif //NETTLE_NT_FEC_PROCESSOR_H
