/**
 * @author caofuxiang
 *         2015-05-11 16:09:09.
 */
#include <sys/unistd.h>
#include <tunnel/nt_proto.h>
#include <utils/utils.h>
#include "udp_cs.h"

#define UDP_BUFFER_SIZE (64*1024)

typedef struct {
    address_t * server_address;
    int sent;
    int received;
    long begin_time;
} session_t;

static void log_session(session_t * session) {
    long elapsed = current_millis() - session->begin_time;
    log_info("received=%d, sent=%d, elapsed=%ldms",
             session->received,
             session->sent,
             elapsed);
}

static void stdin_callback(dispatcher_t * dispatcher, int fd, uint32_t event, void * data) {
    char c;
    if (read(fd, &c, 1) != 1) {
        return;
    }

    if (c == 'q') {
        dispatcher_stop(dispatcher, 0);
    } else if (c == 'p') {
        log_session((session_t *)data);
    }
}

static void udp_server_handle_read(dispatcher_t * dispatcher, udp_t * udp, void * extra) {
    session_t * session = (session_t *)extra;
    uint8_t buffer[UDP_BUFFER_SIZE];
    address_t src;
    address_t dst;
    size_t received;
    if (udp_recv(udp, buffer, UDP_BUFFER_SIZE, &src, &dst, &received) != 0) {
        log_error("Recv error: %s", strerror(errno));
        dispatcher_stop(dispatcher, errno);
        return;
    }

    session->received ++;

    size_t sent;
    nt_api_header_t * header = (nt_api_header_t *) buffer;
    if (nt_api_header_is_valid(header) && received == nt_api_message_size(header)) {
        uint32_t src_ip = header->src_ip;
        uint16_t src_port = header->src_port;
        header->src_ip = header->dst_ip;
        header->src_port = header->dst_port;
        header->dst_ip = src_ip;
        header->dst_port = src_port;
    }
    if (udp_send(udp, &buffer, received, &dst, &src, &sent) != 0) {
        log_error("Send error: %s", strerror(errno));
        dispatcher_stop(dispatcher, errno);
        return;
    }
    session->sent ++;
}

static void timer_callback(dispatcher_t * dispatcher, void * data) {
    session_t * session = (session_t *)data;
    log_session(session);
}

error_t udp_server(address_t * server_address) {
    log_info("Start server ...");
    session_t session;
    SBUILDER(builder, 1024);
    sbuilder_str(&builder, "address = ");
    sbuilder_address(&builder, server_address);
    log_info(builder.buf);
    session.server_address = server_address;
    session.sent = 0;
    session.received = 0;

    dispatcher_t dispatcher;
    dispatcher_init1(&dispatcher, 100);

    udp_t udp;
    udp_init(&udp, session.server_address, false);

    udp_handler_t udp_handler;
    udp_handler_init(&udp_handler, &udp, &session, udp_server_handle_read, NULL);

    if (dispatcher_add_udp(&dispatcher, &udp_handler) != 0) {
        log_error("Add handler error: %s", strerror(errno));
        return errno;
    }

    handler_t stdin_handler;
    handler_init(&stdin_handler, STDIN_FILENO, EPOLLIN, stdin_callback, &session);
    if (dispatcher_add_handler(&dispatcher, STDIN_FILENO, &stdin_handler) != 0) {
        log_warn("add stdin_handler error: %s", strerror(errno));
    }

    if (dispatcher_add_timer(&dispatcher, 5000, true, timer_callback, &session) != 0) {
        log_error("add timer error.");
        return -1;
    }

    session.begin_time = current_millis();
    error_t error = dispatcher_run(&dispatcher);
    log_session(&session);
    return error;
}
