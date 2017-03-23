/**
 * @author caofuxiang
 *         2015-05-18 09:55:55.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <execinfo.h>
#include <utils/logger.h>
#include "common.h"

void address_init(address_t *address, struct sockaddr * addr, socklen_t addrlen) {
    memcpy(&address->addr, addr, (size_t) addrlen);
    address->addrlen = addrlen;
}

void address_init4(address_t *address, struct in_addr * addr, uint16_t port) {
    zero_data(&address->addr_in);
    address->addr_in.sin_family = AF_INET;
    address->addr_in.sin_port = port;
    address->addr_in.sin_addr = *addr;
    address->addrlen = sizeof address->addr_in;
}

void address_init6(address_t *address, struct in6_addr * addr, uint16_t port) {
    zero_data(&address->addr_in6);
    address->addr_in6.sin6_family = AF_INET6;
    address->addr_in6.sin6_port = port;
    address->addr_in6.sin6_addr = *addr;
    address->addrlen = sizeof address->addr_in6;
}

uint16_t address_get_port(const address_t * address) {
    if (address->addr.sa_family == AF_INET) {
        return address->addr_in.sin_port;
    } else {
        return address->addr_in6.sin6_port;
    }
}

void address_set_port(address_t * address, uint16_t port) {
    if (address->addr.sa_family == AF_INET) {
        address->addr_in.sin_port = port;
    } else {
        address->addr_in6.sin6_port = port;
    }
}

uint32_t address_get_addr4(const address_t *address) {
    if (address->addr.sa_family == AF_INET) {
        return address->addr_in.sin_addr.s_addr;
    } else {
        return 0;
    }
}

void address_set_addr4(address_t *address, uint32_t addr) {
    uint16_t port = address_get_port(address);
    struct in_addr in;
    in.s_addr = addr;
    address_init4(address, &in, port);
}

bool address_from(address_t *address, const char *host /* Nullable */, const char *port /* Nullable */, int socktype /* 0 for any type */) {
    struct addrinfo hints;
    struct addrinfo * result;

    zero_data(&hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    error_t err = getaddrinfo(host, port, &hints, &result);
    if (err != 0) {
        return false;
    }
    address_init(address, result->ai_addr, result->ai_addrlen);
    return true;
}

bool address_to(const address_t *address, char *host /* NULL to ignore it */, socklen_t hostlen, char *port /* NULL to ignore it */, socklen_t portlen) {
    error_t err = getnameinfo(&address->addr, address->addrlen, host, hostlen, port, portlen, NI_NUMERICHOST|NI_NUMERICSERV);
    return err == 0;
}

const char * address_parse(address_t * address, const char * str) {
    char name[101];
    char port[21];
    int read;
    if (sscanf(str, "%100[^:]:%20[0-9]%n", name, port, &read) < 2) {
        return NULL;
    }
    if (address_from(address, name, port, 0)) {
        return str+read;
    }
    return NULL;
}

bool sbuilder_address(sbuilder_t * builder, const address_t *address) {
    char host[100];
    char port[20];
    address_to(address, host, sizeof host, port, sizeof port);
    return sbuilder_format(builder, "%s:%s", host, port);
}

void address_print(const address_t *address) {
    char host[100];
    char port[20];
    address_to(address, host, sizeof host, port, sizeof port);
    printf("%s:%s\n", host, port);
}

ssize_t send_v(int socket, const struct iovec * iov, int iovlen, const address_t * src /* nullable */, const address_t * dst /* nullable */) {
    char msg_control[CMSG_SPACE(sizeof(struct in_pktinfo)) + CMSG_SPACE(sizeof(struct in6_pktinfo))];
    zero_array(msg_control, sizeof msg_control);

    struct msghdr msg;
    if (dst != NULL) {
        msg.msg_name = (void *)&dst->addr;
        msg.msg_namelen = sizeof dst->addr;
    } else {
        msg.msg_name = 0;
        msg.msg_namelen = 0;
    }
    msg.msg_iov = (struct iovec *) iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = msg_control;
    if (src == NULL) {
        msg.msg_controllen = 0;
    } else {
        msg.msg_controllen = sizeof msg_control;
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (src->addr.sa_family == AF_INET) {
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IP_PKTINFO;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
            struct in_pktinfo *pktinfo = (struct in_pktinfo*) CMSG_DATA(cmsg);
            pktinfo->ipi_spec_dst = src->addr_in.sin_addr;
            pktinfo->ipi_ifindex = 0;
            msg.msg_controllen = CMSG_SPACE(sizeof(struct in_pktinfo));
        } else if (src->addr.sa_family == AF_INET6) {
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IPV6_PKTINFO;
            cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
            struct in6_pktinfo *pktinfo = (struct in6_pktinfo*) CMSG_DATA(cmsg);
            pktinfo->ipi6_addr = src->addr_in6.sin6_addr;
            pktinfo->ipi6_ifindex = 0;
            msg.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
        } else {
            msg.msg_controllen = 0;
        }
    }
    msg.msg_flags = 0;

    return sendmsg(socket, &msg, 0);
}

ssize_t recv_v(int socket, struct iovec * iov, int iovlen, address_t * src /* out, nullable */, address_t * dst /* out, nullable */) {
    char msg_control[CMSG_SPACE(sizeof(struct in_pktinfo)) + CMSG_SPACE(sizeof(struct in6_pktinfo))];

    struct msghdr msg;
    if (src != NULL) {
        msg.msg_name = &src->addr;
        msg.msg_namelen = sizeof src->addr;
    } else {
        msg.msg_name = 0;
        msg.msg_namelen = 0;
    }
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = msg_control;
    msg.msg_controllen = sizeof msg_control;
    msg.msg_flags = 0;

    ssize_t s = recvmsg(socket, &msg, 0);
    if (s < 0) {
        return s;
    }

    if (src != NULL) {
        src->addrlen = msg.msg_namelen;
    }

    if (dst != NULL) {
        for(struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IP) {
                if (cmsg->cmsg_type == IP_PKTINFO) {
                    struct in_pktinfo *pktinfo = (struct in_pktinfo *) CMSG_DATA(cmsg);
                    address_init4(dst, &pktinfo->ipi_spec_dst, 0);
                    break;
                } else if (cmsg->cmsg_type == IPV6_PKTINFO) {
                    struct in6_pktinfo *pktinfo = (struct in6_pktinfo *) CMSG_DATA(cmsg);
                    address_init6(dst, &pktinfo->ipi6_addr, 0);
                    break;
                }
            }
        }
    }

    return s;
}

error_t enable_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return errno;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        return errno;
    }
    return 0;
}

pid_t current_pid() {
    return getpid();
}

pthread_t current_tid() {
    return pthread_self();
}

void print_backtrace() {
    void *stacks[128];
    int count = backtrace(stacks, sizeof stacks);
    char **symbols = backtrace_symbols(stacks, count);
    SBUILDER(builder, 2048);
    sbuilder_format(&builder, "Backtrace in process %d, thread %lx:", current_pid(), current_tid());
    for (int i = 0; i < count; ++i) {
        sbuilder_format(&builder, "\n\t%s", symbols[i]);
    }
    free(symbols);
    fputs(builder.buf, stderr);
}

error_t signal_handler(int signum, void (*handler)(int)) {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    if (sigaction(signum, &act, NULL) == 0) {
        return 0;
    }
    return errno;
}

static void signal_backtrace(int signum) {
    log_error("Signal caught: %s (%d).", strsignal(signum), signum);
    print_backtrace();
    SIG_DFL(signum);
}

void set_signal_backtrace(int signum) {
    ensure(signal_handler(signum, signal_backtrace) == 0);
}
