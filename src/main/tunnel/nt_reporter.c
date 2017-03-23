/**
 * @author caofuxiang
 *         2015-07-17 09:54:54.
 */

#include <utils/logger.h>
#include <ctype.h>
#include "nt_reporter.h"

static void udp_handle_read(dispatcher_t * dispatcher, udp_t * udp, void * extra) {
    char buffer[100];
    address_t src;
    size_t received;
    error_t err = udp_recvfrom(udp, buffer, array_size(buffer)-1, &src, &received);
    if (err != 0) return;
    while (received >= 1) {
        if (!isspace(buffer[received-1])) {
            break;
        }
        --received;
    }
    buffer[received] = '\0';
    if (strcmp("perf", buffer) == 0) {
        nt_reporter_t * reporter = extra;
        SBUILDER(builder, 4096);
        sbuilder_reset(&builder);
        sbuilder_perf(&builder, reporter->perf);
        log_info("[Command: perf] Perf Counters = %s", builder.buf);
        size_t sent;
        udp_sendto(udp, builder.buf, sbuilder_len(&builder), &src, &sent);
    }
}

static void timer_callback(dispatcher_t * dispatcher, void * data) {
    nt_reporter_t * reporter = data;
    SBUILDER(builder, 4096);
    sbuilder_reset(&builder);
    sbuilder_perf(&builder, reporter->perf);
    log_info("Perf Counters = %s", builder.buf);
}

error_t nt_reporter_init(nt_reporter_t * reporter, address_t * local_address, perf_t * perf) {
    error_t err = dispatcher_init1(&reporter->dispatcher, 100);
    if (err != 0) return err;

    err = udp_init(&reporter->udp, local_address, false);
    if (err != 0) return err;

    udp_handler_init(&reporter->udp_handler, &reporter->udp, reporter, udp_handle_read, NULL);

    err = dispatcher_add_udp(&reporter->dispatcher, &reporter->udp_handler);
    if (err != 0) return err;

    err = dispatcher_add_timer(&reporter->dispatcher, 60000, true, timer_callback, reporter);
    if (err != 0) return err;

    reporter->perf = perf;
    return 0;
}

error_t nt_reporter_destroy(nt_reporter_t * reporter) {
    error_t err = dispatcher_destroy(&reporter->dispatcher);
    if (err != 0) return err;
    return udp_destroy(&reporter->udp);
}

error_t nt_reporter_run(nt_reporter_t * reporter) {
    return dispatcher_run(&reporter->dispatcher);
}
