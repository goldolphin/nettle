/**
 * @author caofuxiang
 *         2015-10-27 16:28:28.
 */

#include "tun_tap.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

error_t tun_tap_init(tun_tap_t * tun_tap, const char *name, bool is_tap, bool no_pi) {
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);
    if(fd < 0 ) {
        return errno;
    }

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    if (is_tap) {
        ifr.ifr_flags = IFF_TAP;
    } else {
        ifr.ifr_flags = IFF_TUN;
    }

    if (no_pi) {
        ifr.ifr_flags |= IFF_NO_PI;
    }

    if (name != NULL) {
        strncpy(ifr.ifr_name, name, TUN_TAP_NAME_SIZE);
    }

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        close(fd);
        return errno;
    }

    tun_tap->fd = fd;
    strncpy(tun_tap->name, ifr.ifr_name, TUN_TAP_NAME_SIZE);
    tun_tap->name[TUN_TAP_NAME_SIZE-1] = '\0';

    return 0;
}

error_t tun_tap_set_persist(tun_tap_t * tun_tap, bool enable) {
    int v = enable ? 1 : 0;
    if (ioctl(tun_tap->fd, TUNSETPERSIST, v) < 0) {
        return errno;
    }
    return 0;
}

error_t tun_tap_close(tun_tap_t * tun_tap) {
    if (close(tun_tap->fd) < 0) {
        return errno;
    }
    return 0;
}
