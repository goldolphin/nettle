/**
 * @author caofuxiang
 *         2015-05-11 16:09:09.
 */
#include <utils/utils.h>
#include <sys/unistd.h>
#include <dispatcher/udp.h>
#include <tunnel/nt_distributor.h>
#include <tunnel/nt_proto.h>
#include <utils/bitmap.h>
#include <utils/metric.h>
#include "packet.h"
#include "udp_cs.h"

#define UDP_BUFFER_SIZE (64*1024)

typedef struct {
    nt_fec_t * fec;
    nt_distributor_t * distributor;
    udp_t * udp;

    address_t *tun_addr;
    nt_encrypt_type_t encrypt_type;

    int count;
    int qps;

    int sent;
    int encoded_sent;
    int received;
    int invalid;
    int duplicated;

    metric_t * metric;

    bitmap_t * response_map;

    long begin_time;
} session_t;

static void log_session(session_t * session) {
    long elapsed = current_millis() - session->begin_time;
    // NOTE: Be careful to modifiy this, which is used to report perf about tunnel status.
    // see: bin/monitor_perf.sh
    log_info("Metric results"
             ": sent=%d, received=%d, invalid=%d, duplicated=%d, packet_loss=%.2f%%"
             ", recv_qps=%.2f, latency.mean=%.2fms, latency.99=%ldms, latency.90=%ldms, latency.min=%ldms, latency.max=%ldms"
             ", elapsed=%ldms",
             session->sent,
             session->received,
             session->invalid,
             session->duplicated,
             100.0*(session->sent-session->received)/session->sent,

             metric_qps(session->metric),
             metric_mean(session->metric),
             metric_ratio(session->metric, 0.99),
             metric_ratio(session->metric, 0.90),
             metric_min(session->metric),
             metric_max(session->metric),

             elapsed);
}

static void stdin_callback(dispatcher_t * dispatcher, int fd, uint32_t event, void * data) {
    char c;
    int ret = read(fd, &c, 1);
    if (ret != 1) {
        if (ret < 0) {
            log_warn("Read error: %s (%d)", strerror(errno), errno);
        }
        dispatcher_remove_handler(dispatcher, fd);
        return;
    }

    if (c == 'q') {
        dispatcher_stop(dispatcher, 0);
    } else if (c == 'p') {
        log_session((session_t *)data);
    }
}

static error_t on_encode(void * extra, nt_fec_header_t * header, uint8_t * payload, int payload_size) {
    session_t *session = extra;
    size_t sent;
    for (int i = 0; ; ++i) {
        nt_path_t * path = nt_distributor_select(session->distributor, header->block_index, i);
        if (path == NULL) break;
        nt_api2_header_t api_header;
        struct iovec iov[3];
        iov[0].iov_base = &api_header;
        iov[0].iov_len = sizeof(nt_api2_header_t);
        iov[1].iov_base = header;
        iov[1].iov_len = sizeof(nt_fec_header_t);
        iov[2].iov_base = payload;
        iov[2].iov_len = (size_t) payload_size;

        nt_api2_header_init(&api_header, (uint32_t) session->encoded_sent++,
                            0, 0, address_get_addr4(&path->dst), address_get_port(&path->dst),
                           session->encrypt_type, 8, (uint16_t) (iov[1].iov_len + iov[2].iov_len));
        address_t * dst = session->tun_addr == NULL ? &path->dst : session->tun_addr;
        int size = nt_api2_message_size(&api_header);
        error_t err = udp_send_v(session->udp, iov, array_size(iov), &path->src, dst, &sent);
        if (err != 0) {
            log_error("Send error: %s (%d)", strerror(err), err);
        }
        ensure(err == 0 && sent == size);
    }
    return 0;
}

static error_t on_decode(void * extra, uint8_t * decoded, int size) {
    session_t * session = extra;
    packet_t * packet = (packet_t *) decoded;
    long now = current_millis();
    if (size < sizeof packet || packet->id < 0 || packet->id >= session->sent || packet->timestamp > now || !packet_check_checksum(
            packet)) {
        log_warn("Invalid message received: size = %d, id = %d, timestamp=%ld, checksum=%lx.", size, packet->id, packet->timestamp, packet->checksum);
        session->invalid++;
    } else if (bitmap_isset(session->response_map, (size_t) packet->id)) {
        // log_warn("Duplicated message received: len = %d, id = %d.", received, packet->id);
        session->duplicated ++;
    } else {
        bitmap_set(session->response_map, (size_t) packet->id);
        session->received++;
        metric_record(session->metric, now - packet->timestamp);
        int k = (session->count+4)/5;
        if (session->received % k == 0) {
            log_info("%d messages returned", session->received);
        }
    }
    return 0;
}

static void udp_client_handle_read(dispatcher_t * dispatcher, udp_t * udp, void * extra) {
    session_t * session = (session_t *)extra;
    uint8_t buffer[UDP_BUFFER_SIZE];
    address_t src;
    address_t dst;
    size_t received;
    error_t error = udp_recv(udp, &buffer, UDP_BUFFER_SIZE, &src, &dst, &received);
    if (error == 0) {
        nt_api_header_t * api_header = (nt_api_header_t *) buffer;
        if (nt_api_header_is_valid(api_header) && nt_api_message_size(api_header) == received) {
            nt_fec_header_t *fec_header = (nt_fec_header_t *) nt_api_message_payload(api_header);
            if (nt_fec_message_size(fec_header) == nt_api_header_payload_size(api_header)) {
                error = nt_fec_decode(session->fec, fec_header, nt_fec_message_payload(fec_header), nt_fec_header_payload_size(fec_header), session, on_decode);
            } else {
                log_error("Recv error: received_encoded_size=%lu, expected_encoded_size=%lu, fec_tag=%u", nt_api_header_payload_size(api_header),
                          nt_fec_message_size(fec_header), fec_header->tag);
            }
        } else {
            log_error("Recv error: received_size=%lu, expected_size=%lu, api_tag=%u", received, nt_api_message_size(api_header), api_header->tag);
        }
    }
    if (error != 0) {
        log_error("Recv error: %s (%d)", strerror(error), error);
        dispatcher_stop(dispatcher, error);
        return;
    }
    if (session->received == session->count) {
        log_info("All messages returned.");
        dispatcher_stop(dispatcher, 0);
    }
}

static void timer_callback(dispatcher_t * dispatcher, void * data) {
    log_warn("Time is out.");
    dispatcher_stop(dispatcher, 0);
}

static void udp_client_handle_write(dispatcher_t * dispatcher, void * data) {
    udp_handler_t * handler = data;
    session_t * session = handler->extra;
    int to_send = session->count - session->sent;
    if (to_send == 0) {
        return;
    }

    long now = current_millis();
    int batch = (int)((now - session->begin_time) * session->qps / 1000)-session->sent;
    if (batch > to_send) {
        batch = to_send;
    }

    if (batch == 0) {
        return;
    }

    packet_t buffer;
    for (int i = 0; i < batch; ++i) {
        packet_init(&buffer, session->sent, current_millis());
        error_t error = nt_fec_encode(session->fec, (uint8_t *) &buffer, sizeof(packet_t), session, on_encode);
        if (error != 0) {
            log_error("Send error: %s", strerror(error));
            dispatcher_stop(dispatcher, error);
            return;
        }
        session->sent ++;
        int k = (session->count+4)/5;
        if (session->sent % k == 0) {
            log_info("%d messages sent", session->sent);
        }
    }

    if (session->sent == session->count) {
        log_info("All messages sent.");
        if (dispatcher_add_timer(dispatcher, 2000, false, timer_callback, NULL) != 0) {
            log_error("add timer error.");
            dispatcher_stop(dispatcher, -1);
        }
    }
}

error_t udp_client(nt_fec_t * fec, nt_distributor_t * distributor, int count, int qps, address_t * tun_addr,
                   nt_encrypt_type_t encrypt_type) {
    log_info("Start client ...");
    session_t session;
    session.fec = fec;
    session.distributor = distributor;

    SBUILDER(builder, 1024);
    for (int i = 0; i < distributor->path_num; ++i) {
        if (i > 0) sbuilder_str(&builder, ", ");
        sbuilder_path(&builder, &session.distributor->paths[i]);
    }
    log_info("Paths: %s", builder.buf);

    if (tun_addr != NULL) {
        sbuilder_reset(&builder);
        sbuilder_address(&builder, tun_addr);
        log_info("Tunnel: %s", builder.buf);
    }
    session.tun_addr = tun_addr;
    session.encrypt_type = encrypt_type;

    session.count = count;
    session.qps = qps;
    session.sent = 0;
    session.encoded_sent = 0;
    session.received = 0;
    session.invalid = 0;
    session.duplicated = 0;

    dispatcher_t dispatcher;
    dispatcher_init1(&dispatcher, 100);

    udp_t udp;
    udp_init(&udp, NULL, false);

    udp_handler_t udp_handler;
    udp_handler_init(&udp_handler, &udp, &session, udp_client_handle_read, NULL);
    int delay = qps > 1000 ? 1 : 1000/qps;
    ensure(dispatcher_add_timer(&dispatcher, delay, true, udp_client_handle_write, &udp_handler) == 0);
    if (dispatcher_add_udp(&dispatcher, &udp_handler) != 0) {
        log_error("add handler error: %s", strerror(errno));
        return errno;
    }

    handler_t stdin_handler;
    handler_init(&stdin_handler, STDIN_FILENO, EPOLLIN, stdin_callback, &session);
    if (dispatcher_add_handler(&dispatcher, STDIN_FILENO, &stdin_handler) != 0) {
        log_warn("add stdin_handler error: %s, in daemon mode?", strerror(errno));
    }

    session.udp = &udp;
    session.metric = make_metric(1000);

    session.response_map = make_bitmap((size_t) count);

    session.begin_time = current_millis();
    error_t error = dispatcher_run(&dispatcher);
    log_session(&session);
    destroy_bitmap(session.response_map);
    return error;
}
