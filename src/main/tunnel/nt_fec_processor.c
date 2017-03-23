/**
 * @author caofuxiang
 *         2015-07-01 10:17:17.
 */

#include <utils/logger.h>
#include "nt_fec_processor.h"
#include "nt_rs_fec.h"
#include "nt_proto.h"
#include "nt_fec.h"
#include "nt_distributor.h"
#include "nt_perf.h"
#include "nt_proto2.h"

static error_t on_encode(void * extra, nt_fec_header_t * header, uint8_t * payload, int payload_size) {
    nt_fec_processor_t *processor = extra;
    size_t sent;
    struct iovec iov[2];
    iov[0].iov_base = header;
    iov[0].iov_len = sizeof(nt_fec_header_t);
    iov[1].iov_base = payload;
    iov[1].iov_len = (size_t) payload_size;
    for (int i = 0; ; ++i) {
        nt_path_t *path = nt_distributor_select(processor->distributor, header->block_index, i);
        if (path == NULL) break;
        error_t err = udp_send_v(processor->tun_udp, iov, array_size(iov), &path->src, &path->dst, &sent);
        if (err == 0) {
            ensure(sent == nt_fec_message_size(header));
            perf_inc(processor->perf, COUNTER_TUN_SENT, 1);
            perf_inc(processor->perf, COUNTER_TUN_SENT_BYTES, (int) sent);
        } else {
            perf_inc(processor->perf, COUNTER_TUN_SEND_ERROR, 1);
            log_warn("Send error in on_encode: %s (%d)", strerror(err), err);
        }
    }
    return 0;
}

static error_t on_decode(void * extra, uint8_t * decoded, int size) {
    nt_fec_processor_t *processor = extra;
    if (nt_api_header_is_valid(decoded)) {
        nt_api_header_t * header = (nt_api_header_t *) decoded;
        if (size != nt_api_message_size(header)) {
            perf_inc(processor->perf, COUNTER_CORRUPT_DECODED_DATA, 1);
            return 0;
        }
        address_t dst;
        address_init4(&dst, (struct in_addr *) &header->dst_ip, header->dst_port);
        size_t sent;
        if (udp_sendto(processor->api_udp, decoded, (size_t) size, &dst, &sent) == 0) {
            ensure(sent == size);
            perf_inc(processor->perf, COUNTER_API_SENT_V1, 1);
            perf_inc(processor->perf, COUNTER_API_SENT, 1);
            perf_inc(processor->perf, COUNTER_API_SENT_BYTES, (int) sent);
        } else {
            perf_inc(processor->perf, COUNTER_API_SEND_ERROR, 1);
        }
    } else if (nt_api2_header_is_valid(decoded)) {
        nt_api2_header_t * header = (nt_api2_header_t *) decoded;
        if (size != nt_api2_message_size(header)) {
            perf_inc(processor->perf, COUNTER_CORRUPT_DECODED_DATA, 1);
            return 0;
        }
        if (header->ttl <= 0) {
            perf_inc(processor->perf, COUNTER_TTL_EXCEEDED, 1);
            return 0;
        }
        -- header->ttl;

        void * payload;
        switch (header->encrypt_type) {
            case NT_ENCRYPT_TYPE_NONE:
                payload = nt_api2_message_payload(header);
                break;
            case NT_ENCRYPT_TYPE_KC: {
                size_t decrypted_size = 0;
                const char *decrypted = NULL;
                if (processor->encryptor != NULL) {
                    decrypted = ka_encryptor_decrypt(processor->encryptor,
                                                     nt_api2_message_payload(header),
                                                     nt_api2_header_payload_size(header), &decrypted_size);
                }
                if (decrypted == NULL) {
                    perf_inc(processor->perf, COUNTER_DECRYPT_ERROR, 1);
                    return 0;
                } else {
                    payload = (void *) decrypted;
                    nt_api2_header_set_payload_size(header, (uint16_t) decrypted_size);
                }
                break;
            }
            default:
                ensure(false); // Should not happen because the header is "valid".
        }

        address_t dst;
        address_init4(&dst, (struct in_addr *) &header->dst_ip, header->dst_port);
        nt_api_header_t header1;
        nt_api_header_from(&header1, header);

        struct iovec iov[2];
        iov[0].iov_base = &header1;
        iov[0].iov_len = sizeof(header1);
        iov[1].iov_base = payload;
        iov[1].iov_len = (size_t) nt_api2_header_payload_size(header);

        size_t sent;
        error_t err = udp_send_v(processor->api_udp, iov, array_size(iov), NULL, &dst, &sent);
        if (err == 0) {
            ensure(sent == nt_api_message_size(&header1));
            perf_inc(processor->perf, COUNTER_API_SENT_V1, 1);
            perf_inc(processor->perf, COUNTER_API_SENT, 1);
            perf_inc(processor->perf, COUNTER_API_SENT_BYTES, (int) sent);
        } else {
            perf_inc(processor->perf, COUNTER_API_SEND_ERROR, 1);
            log_warn("Send error in on_decode: %s (%d)", strerror(err), err);
        }
    } else {
        perf_inc(processor->perf, COUNTER_CORRUPT_DECODED_DATA, 1);
    }
    return 0;
}

static void encoder_on_message(dispatcher_t * dispatcher, worker_t * worker, void * extra, void * message) {
    nt_fec_processor_t *processor = extra;
    if (nt_api_header_is_valid(message)) {
        nt_api_header_t * header = (nt_api_header_t *) message;
        ensure(nt_fec_encode(processor->fec, message, nt_api_message_size(header), processor, on_encode) == 0);
    } else if (nt_api2_header_is_valid(message)) {
        nt_api2_header_t * header = (nt_api2_header_t *) message;
        switch (header->encrypt_type) {
            case NT_ENCRYPT_TYPE_NONE:
                ensure(nt_fec_encode(processor->fec, message, nt_api2_message_size(header), processor, on_encode) == 0);
                break;
            case NT_ENCRYPT_TYPE_KC: {
                size_t encrypted_size = 0;
                const char *encrypted = NULL;
                if (processor->encryptor != NULL) {
                    encrypted = ka_encryptor_encrypt(processor->encryptor,
                                         nt_api2_message_payload(header),
                                         nt_api2_header_payload_size(header), &encrypted_size);
                }
                if (encrypted == NULL) {
                    perf_inc(processor->perf, COUNTER_ENCRYPT_ERROR, 1);
                } else {
                    uint8_t buf[sizeof(nt_api2_header_t) + encrypted_size];
                    memcpy(buf, header, sizeof(nt_api2_header_t));
                    nt_api2_header_set_payload_size((nt_api2_header_t *) buf, (uint16_t) encrypted_size);
                    memcpy(nt_api2_message_payload((nt_api2_header_t *) buf), encrypted, encrypted_size);
                    ensure(nt_fec_encode(processor->fec, buf, nt_api2_message_size((nt_api2_header_t *) buf),
                                         processor, on_encode) == 0);
                }
                break;
            }
            default:
                ensure(false); // Should not happen because the header is "valid".
        }
    } else {
        ensure(false); // Should not happen and be handled by upstreams(the listener).
    }
}

static void decoder_on_message(dispatcher_t * dispatcher, worker_t * worker, void * extra, void * message) {
    nt_fec_header_t * header = (nt_fec_header_t *) message;
    nt_fec_processor_t *processor = extra;

    error_t err = nt_fec_decode(processor->fec, header, nt_fec_message_payload(header), nt_fec_header_payload_size(header), processor, on_decode);
    switch (err) {
        case 0:
            break;
        case E_FEC_DECODER_BUFFER_FULL:
            perf_inc(processor->perf, COUNTER_DECODER_BUFFER_FULL, 1);
            break;
        case E_FEC_INVALID_HEADER:
            perf_inc(processor->perf, COUNTER_INVALID_ENCODED_DATA, 1);
            break;
        default:
            perf_inc(processor->perf, COUNTER_DECODE_ERROR, 1);
    }
}

error_t nt_fec_processor_init(nt_fec_processor_t *processor, size_t mq_capacity,
                              nt_distributor_t *distributor, udp_t * api_udp, udp_t * tun_udp,
                              nt_fec_t *fec, ka_encryptor_t * encryptor, perf_t * perf) {
    error_t err = worker_init(&processor->encoder, mq_capacity,
                              processor,
                              encoder_on_message);
    if (err != 0) return err;

    err = worker_init(&processor->decoder, mq_capacity,
                      processor,
                      decoder_on_message);
    if (err != 0) return err;

    err = dispatcher_init1(&processor->dispatcher, 100);
    if (err != 0) return err;

    err = dispatcher_add_worker(&processor->dispatcher, &processor->encoder);
    if (err != 0) return err;

    err = dispatcher_add_worker(&processor->dispatcher, &processor->decoder);
    if (err != 0) return err;

    processor->distributor = distributor;
    processor->api_udp = api_udp;
    processor->tun_udp = tun_udp;
    processor->fec = fec;
    processor->encryptor = encryptor;
    processor->perf = perf;
    return 0;
}

error_t nt_fec_processor_destroy(nt_fec_processor_t *processor) {
    error_t err = dispatcher_destroy(&processor->dispatcher);
    if (err != 0) return err;

    err = worker_destroy(&processor->decoder);
    if (err != 0) return err;

    err = worker_destroy(&processor->encoder);
    if (err != 0) return err;

    return 0;
}

error_t nt_fec_processor_run(nt_fec_processor_t *processor) {
    return dispatcher_run(&processor->dispatcher);
}

bool nt_fec_processor_encode(nt_fec_processor_t *processor, void * message, worker_free_t free) {
    return worker_send(&processor->encoder, message, free);
}

bool nt_fec_processor_decode(nt_fec_processor_t *processor, void * message, worker_free_t free) {
    return worker_send(&processor->decoder, message, free);
}
