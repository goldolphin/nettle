/**
 * @author caofuxiang
 *         2015-07-17 09:54:54.
 */

#ifndef NETTLE_NT_REPORTER_H
#define NETTLE_NT_REPORTER_H

#include <dispatcher/udp.h>
#include <dispatcher/perf.h>

typedef struct {
    dispatcher_t dispatcher;
    udp_t udp;
    udp_handler_t udp_handler;
    perf_t * perf;
} nt_reporter_t;

error_t nt_reporter_init(nt_reporter_t * reporter, address_t * local_address, perf_t * perf);

error_t nt_reporter_destroy(nt_reporter_t * reporter);

error_t nt_reporter_run(nt_reporter_t * reporter);

#endif //NETTLE_NT_REPORTER_H
