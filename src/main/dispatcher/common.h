/**
 * @author caofuxiang
 *         2015-05-18 09:55:55.
 */

#ifndef NETTLE_COMMON_H
#define NETTLE_COMMON_H

#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <signal.h>
#include <utils/memory.h>
#include <utils/sbuilder.h>

#ifndef ERROR_T_DEFINED
#define ERROR_T_DEFINED
typedef int error_t;
#endif

typedef struct {
    union {
        struct sockaddr addr;
        struct sockaddr_in addr_in;
        struct sockaddr_in6 addr_in6;
    };
    socklen_t addrlen;
} address_t;

void address_init(address_t *address, struct sockaddr * addr, socklen_t addrlen);

void address_init4(address_t *address, struct in_addr * addr, uint16_t port);

void address_init6(address_t *address, struct in6_addr * addr, uint16_t port);

uint16_t address_get_port(const address_t * address);

void address_set_port(address_t * address, uint16_t port);

uint32_t address_get_addr4(const address_t *address);

void address_set_addr4(address_t *address, uint32_t addr);

bool address_from(address_t *address, const char *host /* Nullable */, const char *port /* Nullable */, int socktype /* 0 for any type */);

bool address_to(const address_t *address, char *host /* NULL to ignore it */, socklen_t hostlen, char *port /* NULL to ignore it */, socklen_t portlen);

const char * address_parse(address_t * address, const char * str);

bool sbuilder_address(sbuilder_t * builder, const address_t *address);

void address_print(const address_t *address);

ssize_t send_v(int socket, const struct iovec * iov, int iovlen, const address_t * src /* nullable */, const address_t * dst);

ssize_t recv_v(int socket, struct iovec * iov, int iovlen, address_t * src /* out */, address_t * dst /* out, nullable */);

error_t enable_nonblock(int fd);

pid_t current_pid();

pthread_t current_tid();

void print_backtrace();

error_t signal_handler(int signum, void (*handler)(int));

void set_signal_backtrace(int signum);

#endif //NETTLE_COMMON_H
