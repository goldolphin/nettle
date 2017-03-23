/**
 * @author caofuxiang
 *         2015-10-28 16:04:04.
 */

#include <dispatcher/common.h>
#include <dispatcher/tun_tap.h>
#include <dispatcher/udp.h>
#include <unistd.h>
#include <utils/logger.h>
#include <tunnel/nt_client.h>
#include "nt-proxy.h"

static void tun_tap_handle_read(dispatcher_t * dispatcher, int fd, uint32_t event, void * data) {
    nt_proxy_t * proxy = data;
    char buf[proxy->tun_tap_buffer_size];
    ssize_t received = read(fd, buf, sizeof(buf));
    if (received < 0) {
        log_error("Error on reading tuntap: %s", strerror(errno));
        ensure(false);
    }
    struct tun_pi * pi = (struct tun_pi *) buf;
    if (pi->flags & TUN_PKT_STRIP) {
        log_warn("Receive buffer is too small");
        return;
    }

    address_t peer_addr;
    if (pi->proto == htons(ETH_P_IP)) {
        struct iphdr * header = (struct iphdr *) (buf + sizeof(struct tun_pi));
        address_init4(&peer_addr, (struct in_addr * )&header->daddr, address_get_port(&proxy->local_addr));
    } else if (pi->proto == htons(ETH_P_IPV6)) {
        struct ip6_hdr * header = (struct ip6_hdr *) (buf + sizeof(struct tun_pi));
        address_init6(&peer_addr, &header->ip6_dst, address_get_port(&proxy->local_addr));
    } else {
        log_warn("Unsupported protocol: %X", pi->proto);
        return;
    }

    size_t sent;
    error_t err = udp_sendto_tunnel(&proxy->udp, buf, (size_t) received, &peer_addr, idgen_next(&proxy->idgen), &proxy->tunnel_addr, &sent);
    if (err != 0) {
        log_warn("Error on sending to tunnel: %s", strerror(err));
        return;
    }
}

static void udp_handle_read(dispatcher_t * dispatcher, udp_t * udp, void * extra) {
    nt_proxy_t * proxy = extra;
    char buf[proxy->tun_tap_buffer_size];
    address_t src_addr;
    uint32_t id;
    address_t tunnel_addr;
    size_t received;
    error_t err = udp_recvfrom_tunnel(udp, buf, sizeof(buf),&src_addr, &id, &tunnel_addr, &received);
    if (err != 0) {
        log_warn("Error on receiving from tunnel: %s", strerror(err));
        return;
    }
    ssize_t sent = write(proxy->tun_tap.fd, buf, received);
    if (sent < 0) {
        log_error("Error on writing tuntap: %s", strerror(errno));
        ensure(false);
    }
}

error_t nt_proxy_init(nt_proxy_t * proxy, const char * tun_tap_name, address_t * local_addr, address_t * tunnel_addr, int tun_tap_buffer_size) {
    error_t err = tun_tap_init(&proxy->tun_tap, tun_tap_name, false, false);
    if (err != 0) return err;
    err = udp_init(&proxy->udp, local_addr, false);
    if (err != 0) {
        ensure(tun_tap_close(&proxy->tun_tap) == 0);
        return err;
    }
    handler_init(&proxy->tun_tap_handler, proxy->tun_tap.fd, EPOLLIN, tun_tap_handle_read, proxy);
    udp_handler_init(&proxy->udp_handler, &proxy->udp, proxy, udp_handle_read, NULL);
    proxy->tunnel_addr = *tunnel_addr;
    proxy->local_addr = *local_addr;
    proxy->tun_tap_buffer_size = tun_tap_buffer_size;

    idgen_init(&proxy->idgen, 0);
    return 0;
}

error_t nt_proxy_destroy(nt_proxy_t * proxy) {
    error_t err = tun_tap_close(&proxy->tun_tap);
    if (err != 0) return err;
    return udp_destroy(&proxy->udp);
}

error_t dispatcher_add_nt_proxy(dispatcher_t * dispatcher, nt_proxy_t * proxy) {
    error_t err = dispatcher_add_handler(dispatcher, proxy->tun_tap_handler.fd, &proxy->tun_tap_handler);
    if (err != 0) return err;
    err = dispatcher_add_udp(dispatcher, &proxy->udp_handler);
    if (err != 0) {
        ensure(dispatcher_remove_handler(dispatcher, proxy->tun_tap_handler.fd) == 0);
    }
    return err;
}
