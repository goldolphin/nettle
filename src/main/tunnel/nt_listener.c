/**
 * @author caofuxiang
 *         2015-07-01 17:27:27.
 */

#include "nt_listener.h"
#include "nt_proto2.h"

error_t nt_listener_init(nt_listener_t * listener, udp_t * udp, size_t pool_capacity, size_t max_buffer_size,
                         void * extra, nt_listener_handle_t handle_read /* nullable */) {
    error_t err = dispatcher_init1(&listener->dispatcher, 100);
    if (err != 0) return err;

    udp_handler_init(&listener->handler, udp, listener, (udp_handle_read_t) handle_read, NULL);

    err = dispatcher_add_udp(&listener->dispatcher, &listener->handler);
    if (err != 0) return err;

    if (pool_capacity > 0) {
        pool_init(&listener->pool, pool_capacity, max_buffer_size);
    }

    listener->max_buffer_size = max_buffer_size;
    listener->pool_capacity = pool_capacity;
    listener->extra = extra;
    return 0;
}

error_t nt_listener_destroy(nt_listener_t * listener) {
    if (listener->pool_capacity > 0) {
        pool_destroy(&listener->pool);
    }
    return dispatcher_destroy(&listener->dispatcher);
}

error_t nt_listener_run(nt_listener_t * listener) {
    return dispatcher_run(&listener->dispatcher);
}

void nt_fec_listener_context_init(nt_fec_listener_context_t *context, nt_fec_processor_t *processors, int processor_num,
                                  nt_forwarder_t * forwarder, perf_t *perf) {
    context->processors = processors;
    context->processor_num = processor_num;
    context->last_processor = 0;
    context->forwarder = forwarder;
    context->perf = perf;
}

void nt_api_handle_read(dispatcher_t * dispatcher, udp_t * udp, nt_listener_t * listener) {
    nt_fec_listener_context_t *context = listener->extra;

    void * buffer = pool_new(&listener->pool);
    if (buffer == NULL) {
        return;
    }

    address_t src;
    size_t received;
    error_t err = udp_recvfrom(udp, buffer, listener->max_buffer_size, &src, &received);
    if (err != 0) goto recv_failure;
    uint32_t src_ip = address_get_addr4(&src);
    if (src_ip == 0) goto recv_failure;

    if (nt_api_header_is_valid(buffer)) {
        nt_api_header_t * header = buffer;
        if (nt_api_message_size(header) != received) {
            goto recv_failure;
        }
        header->src_ip = src_ip;
        header->src_port = address_get_port(&src);
        perf_inc(context->perf, COUNTER_API_RECEIVED_V1, 1);
    } else if (nt_api2_header_is_valid(buffer)) {
        nt_api2_header_t * header = buffer;
        if (nt_api2_message_size(header) != received) {
            goto recv_failure;
        }
        header->src_ip = src_ip;
        header->src_port = address_get_port(&src);
    } else {
        goto recv_failure;
    }

    perf_inc(context->perf, COUNTER_API_RECEIVED, 1);
    perf_inc(context->perf, COUNTER_API_RECEIVED_BYTES, (int) received);

    // FEC encoding and transmission.
    if (context->processors != NULL) {
        // Messages come in continuously will be put in a block,
        // and messages in a block should be dispatched to the same encoder.
        // Our strategy is to dispatch messages to the last encoder when it's idle.
        nt_fec_processor_t * processor = &context->processors[context->last_processor];
        if (worker_message_count(&processor->encoder) != 0) {
            context->last_processor = (context->last_processor+1) % context->processor_num;
            processor = &context->processors[context->last_processor];
        }
        if (nt_fec_processor_encode(processor, buffer, pool_free)) {
            return;
        }
        perf_inc(context->perf, COUNTER_ENCODER_BUSY, 1);
    }
    goto cleanup;

    recv_failure:
    perf_inc(context->perf, COUNTER_API_RECEIVE_ERROR, 1);

    cleanup:
    pool_free(buffer);
}

void nt_tun_handle_read(dispatcher_t * dispatcher, udp_t * udp, nt_listener_t * listener) {
    nt_fec_listener_context_t *context = listener->extra;

    void * buffer = pool_new(&listener->pool);
    if (buffer == NULL) {
        return;
    }

    address_t src;
    size_t received;
    error_t err = udp_recvfrom(udp, buffer, listener->max_buffer_size, &src, &received);
    if (err != 0) goto recv_failure;

    // FEC decoding and transmission.
    nt_fec_header_t * header = buffer;
    if (nt_fec_message_size(header) != received) {
        goto recv_failure;
    }

    perf_inc(context->perf, COUNTER_TUN_RECEIVED, 1);
    perf_inc(context->perf, COUNTER_TUN_RECEIVED_BYTES, (int) received);
    // Dispatch messages by its block_id.
    uint32_t block_id = nt_fec_header_block_id(header);
    if (context->processors == NULL) {
        goto recv_failure;
    }
    nt_fec_processor_t * processor = &context->processors[block_id % context->processor_num];
    nt_fec_header_set_block_id(header, block_id/context->processor_num);
    if (nt_fec_processor_decode(processor, buffer, pool_free)) {
        return;
    }
    perf_inc(context->perf, COUNTER_DECODER_BUSY, 1);
    goto cleanup;

    recv_failure:
    perf_inc(context->perf, COUNTER_TUN_RECEIVE_ERROR, 1);

    cleanup:
    pool_free(buffer);
}
