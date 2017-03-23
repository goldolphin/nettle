/**
 * @author caofuxiang
 *         2015-08-18 09:39:39.
 */

#include <utils/lrumap.h>
#include <utils/utils.h>
#include <utils/pair_t.h>
#include "nt_echo_listener.h"
#include "nt_proto.h"

#define UDP_BUFFER_SIZE (64*1024)

static void handle_read(dispatcher_t * dispatcher, udp_t * udp, nt_listener_t * listener) {
    char buffer[UDP_BUFFER_SIZE];
    address_t src;
    address_t dst;
    size_t received;
    if (udp_recv(udp, buffer, UDP_BUFFER_SIZE, &src, &dst, &received) != 0) {
        return;
    }

    size_t sent;
    nt_api_header_t * header = (nt_api_header_t *) buffer;
    if (nt_api_header_is_valid(header) && received == nt_api_message_size(header)) {
        nt_echo_listener_t * echo_listener = listener->extra;
        /// Deduplicate.
        if (lrumap_capacity(&echo_listener->dedup_cache) > 0) {
            pair_t key_value;
            long current = current_millis();
            if (lrumap_get(&echo_listener->dedup_cache, header, &key_value)) {
                if (current - ptr2int(key_value.value, long) > echo_listener->dedup_timeout) {
                    ensure(lrumap_remove(&echo_listener->dedup_cache, key_value.key));
                    pool_free(key_value.key);
                } else {
                    // Duplicate.
                    return;
                }
            } else {
                // Expires the oldest data.
                if (lrumap_size(&echo_listener->dedup_cache) == lrumap_capacity(&echo_listener->dedup_cache)) {
                    ensure(lrumap_peek(&echo_listener->dedup_cache, &key_value));
                    ensure(lrumap_remove(&echo_listener->dedup_cache, key_value.key));
                    pool_free(key_value.key);
                }
            }

            // Insert new entry.
            nt_api_header_t * key = pool_new(&echo_listener->cache_pool);
            memcpy(key, header, sizeof(nt_api_header_t));
            ensure(lrumap_put(&echo_listener->dedup_cache, key, int2ptr(current, void)));
        }

        uint32_t src_ip = header->src_ip;
        uint16_t src_port = header->src_port;
        header->src_ip = header->dst_ip;
        header->src_port = header->dst_port;
        header->dst_ip = src_ip;
        header->dst_port = src_port;
    }
    udp_send(udp, &buffer, received, &dst, &src, &sent);
}

size_t header_hash_func (void * key) {
    nt_api_header_t * header = key;
    size_t h = header->tag;
    h = 31*h + header->id_n;
    h = 31*h + header->src_ip;
    h = 31*h + header->src_port;
    return h;
}

bool header_equal_func (void * key1, void * key2) {
    nt_api_header_t * header1 = key1;
    nt_api_header_t * header2 = key2;
    return header1->tag == header2->tag
           && header1->id_n == header2->id_n
           && header1->src_ip == header2->src_ip
           && header1->src_port == header2->src_port;
}


error_t nt_echo_listener_init(nt_echo_listener_t * listener, udp_t * udp, int dedup_cache_size, long dedup_timeout) {
    lrumap_init1(&listener->dedup_cache, (size_t) dedup_cache_size, header_hash_func, header_equal_func);
    listener->dedup_timeout = dedup_timeout;
    pool_init(&listener->cache_pool, (size_t) dedup_cache_size, sizeof(nt_api_header_t));
    return nt_listener_init(&listener->listener, udp, 0, 0, listener, handle_read);
}

error_t nt_echo_listener_destroy(nt_echo_listener_t * listener) {
    lrumap_destroy(&listener->dedup_cache);
    pool_destroy(&listener->cache_pool);
    return nt_listener_destroy(&listener->listener);
}
