/**
 * @author caofuxiang
 *         2015-08-06 09:52:52.
 */

#ifndef NETTLE_NT_MONITOR_H
#define NETTLE_NT_MONITOR_H

#include <dispatcher/udp.h>
#include <dispatcher/perf.h>

typedef struct {
    dispatcher_t dispatcher;
    udp_handler_t udp_handler;
    udp_t udp;
    size_t packet_size;
    int qps;
    perf_t * perf;

    uint32_t sent;
    uint32_t received;
    uint32_t invalid;
} nt_monitor_t;


#endif //NETTLE_NT_MONITOR_H
