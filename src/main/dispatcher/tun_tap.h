/**
 * @author caofuxiang
 *         2015-10-27 16:28:28.
 */

#ifndef NETTLE_TUN_TAP_H
#define NETTLE_TUN_TAP_H

#include <dispatcher/common.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define TUN_TAP_NAME_SIZE (IFNAMSIZ)

typedef struct {
    int fd;
    char name[TUN_TAP_NAME_SIZE];
} tun_tap_t;

error_t tun_tap_init(tun_tap_t * tun_tap, const char *name, bool is_tap, bool no_pi);

error_t tun_tap_set_persist(tun_tap_t * tun_tap, bool enable);

error_t tun_tap_close(tun_tap_t * tun_tap);

#endif //NETTLE_TUN_TAP_H
