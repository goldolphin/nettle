/**
 * @author caofuxiang
 *         2015-07-01 17:27:27.
 */

#ifndef NETTLE_NT_LISTENER_H
#define NETTLE_NT_LISTENER_H

#include <dispatcher/udp.h>
#include <dispatcher/perf.h>
#include "nt_fec_processor.h"
#include "nt_perf.h"
#include "nt_forwarder.h"

/**
 * Listener definition.
 */
typedef struct {
    dispatcher_t dispatcher;
    udp_handler_t handler;
    pool_t pool;
    size_t max_buffer_size;
    size_t pool_capacity;
    void * extra;
} nt_listener_t;

typedef void (* nt_listener_handle_t)(dispatcher_t * dispatcher, udp_t * udp, nt_listener_t * listener);

error_t nt_listener_init(nt_listener_t * listener, udp_t * udp, size_t pool_capacity, size_t max_buffer_size,
                         void * extra, nt_listener_handle_t handle_read /* nullable */);

error_t nt_listener_destroy(nt_listener_t * listener);

error_t nt_listener_run(nt_listener_t * listener);

/**
 * Customized listener context.
 */
typedef struct {
    nt_fec_processor_t * processors;
    int processor_num;
    int last_processor;
    nt_forwarder_t * forwarder;
    perf_t * perf;
} nt_fec_listener_context_t;

void nt_fec_listener_context_init(nt_fec_listener_context_t *context, nt_fec_processor_t *processors, int processor_num,
                                  nt_forwarder_t * forwarder, perf_t *perf);

void nt_api_handle_read(dispatcher_t * dispatcher, udp_t * udp, nt_listener_t * listener);

void nt_tun_handle_read(dispatcher_t * dispatcher, udp_t * udp, nt_listener_t * listener);

#endif //NETTLE_NT_LISTENER_H
