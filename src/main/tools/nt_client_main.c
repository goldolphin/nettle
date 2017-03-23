#include <stdio.h>
#include <errno.h>
#include <dispatcher/common.h>
#include <utils/logger.h>
#include <tunnel/nt_client.h>
#include <tunnel/nt_distributor.h>
#include <sys/unistd.h>

/**
 * @author caofuxiang
 *         2015-07-30 09:51:51.
 */

#define BUFFER_LEN 4096

typedef struct {
    bool use_tunnel;
    bool legacy_api;
    bool to_encrypt;
    address_t target_addr;
    address_t tunnel_addr;
} options_t;

typedef struct {
    options_t options;
    udp_t udp;

    uint32_t sent;
    uint32_t received;
    bool to_close;
} context_t;

static void timer_callback(dispatcher_t * dispatcher, void * data) {
    log_warn("Time is out.");
    dispatcher_stop(dispatcher, 1);
}

static void stdin_handle_read(dispatcher_t * dispatcher, int fd, uint32_t event, void * data) {
    context_t * context = data;
    char buffer[BUFFER_LEN];
    ssize_t r = read(fd, buffer, sizeof buffer);
    ensure(r >= 0);
    if (r == 0) {
        if (context->received == context->sent) {
            dispatcher_stop(dispatcher, 0);
        } else {
            int timeout = 2000;
            dispatcher_add_timer(dispatcher, timeout, false, timer_callback, context);
            dispatcher_remove_handler(dispatcher, fd);
            context->to_close = true;
        }
        return;
    };
    size_t sent;
    if (context->options.use_tunnel) {
        if (context->options.legacy_api) {
            ensure(udp_sendto_tunnel(&context->udp, buffer, (size_t)r, &context->options.target_addr, context->sent,
                                     &context->options.tunnel_addr, &sent) == 0);
        } else {
            if (context->options.to_encrypt) {
                ensure(udp_sendto_tunnel2(&context->udp, buffer, (size_t)r, &context->options.target_addr, context->sent,
                                          &context->options.tunnel_addr, NT_ENCRYPT_TYPE_KC, 8, &sent) == 0);
            } else {
                ensure(udp_sendto_tunnel2(&context->udp, buffer, (size_t)r, &context->options.target_addr, context->sent,
                                          &context->options.tunnel_addr, NT_ENCRYPT_TYPE_NONE, 8, &sent) == 0);
            }
        }
    } else {
        ensure(udp_sendto(&context->udp, buffer, (size_t)r, &context->options.target_addr, &sent) == 0);
    }
    ++ context->sent;
}

static void udp_handle_read(dispatcher_t * dispatcher, udp_t * udp, void * extra) {
    context_t * context = extra;
    char buffer[BUFFER_LEN];
    address_t from;
    size_t received;
    if (context->options.use_tunnel) {
        uint32_t id;
        address_t tunnel_addr;
        ensure(udp_recvfrom_tunnel(udp, buffer, sizeof buffer, &from, &id, &tunnel_addr, &received) == 0);
    } else {
        ensure(udp_recvfrom(udp, buffer, BUFFER_LEN, &from, &received) == 0);
    }
    ++ context->received;
    if (context->to_close && context->received == context->sent) {
        dispatcher_stop(dispatcher, 0);
    }

    if (received < BUFFER_LEN) {
        buffer[received] = 0;
    } else {
        buffer[received-1] = 0;
    }
    fputs(buffer, stdout);
}

bool options_parse(options_t * options, int argc, char **argv) {
    options->use_tunnel = false;
    options->legacy_api = false;
    options->to_encrypt = false;
    zero_data(&options->target_addr);
    zero_data(&options->tunnel_addr);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--legacy") == 0) {
            options->legacy_api = true;
        } else if (strcmp(argv[i], "--encrypt") == 0) {
            options->to_encrypt = true;
        } else if (strcmp(argv[i], "--target") == 0) {
            ensure(address_parse(&options->target_addr, argv[++i]) != NULL);
        } else if (strcmp(argv[i], "--tunnel") == 0) {
            ensure(address_parse(&options->tunnel_addr, argv[++i]) != NULL);
            options->use_tunnel = true;
        } else {
            return false;
        }
    }

    return address_get_addr4(&options->target_addr) != 0;
}

int main(int argc, char **argv) {
    options_t options;
    if (!options_parse(&options, argc, argv)) {
        fprintf(stderr, "Invalid arguments.\n"
                "Usage: %s [--legacy] [--encrypt] --target <target-address> [--tunnel <tunnel-api-address>]\n", argv[0]);
        return EINVAL;
    }
    SBUILDER(builder, 4096);

    sbuilder_str(&builder, "Target address: ");
    sbuilder_address(&builder, &options.target_addr);
    if (options.use_tunnel) {
        sbuilder_str(&builder, ", Tunnel address: ");
        sbuilder_address(&builder, &options.tunnel_addr);
    }
    log_info(builder.buf);

    context_t context;
    context.options = options;
    context.sent = 0;
    context.received = 0;
    context.to_close = false;

    ensure(udp_init(&context.udp, NULL, false) == 0);

    dispatcher_t dispatcher;
    ensure(dispatcher_init1(&dispatcher, 100) == 0);

    handler_t stdin_handler;
    handler_init(&stdin_handler, STDIN_FILENO, EPOLLIN, stdin_handle_read, &context);
    ensure(dispatcher_add_handler(&dispatcher, STDIN_FILENO, &stdin_handler) == 0);

    udp_handler_t udp_handler;
    udp_handler_init(&udp_handler, &context.udp, &context, udp_handle_read, NULL);
    ensure(dispatcher_add_udp(&dispatcher, &udp_handler) == 0);

    error_t err = dispatcher_run(&dispatcher);
    log_info("Statistics: sent=%u, received=%u", context.sent, context.received);
    return err;
}
