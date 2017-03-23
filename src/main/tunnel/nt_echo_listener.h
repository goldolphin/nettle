/**
 * @author caofuxiang
 *         2015-08-18 09:39:39.
 */

#ifndef NETTLE_NT_ECHO_LISTENER_H
#define NETTLE_NT_ECHO_LISTENER_H

#include "nt_listener.h"

typedef struct {
    nt_listener_t listener;
    pool_t cache_pool;
    lrumap_t dedup_cache;
    long dedup_timeout;
} nt_echo_listener_t;

error_t nt_echo_listener_init(nt_echo_listener_t * listener, udp_t * udp, int dedup_cache_size, long dedup_timeout);

error_t nt_echo_listener_destroy(nt_echo_listener_t * listener);

#endif //NETTLE_NT_ECHO_LISTENER_H
