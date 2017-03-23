/**
 * @author caofuxiang
 *         2015-06-26 15:25:25.
 */

#include "nt_fec.h"

void nt_fec_header_init(nt_fec_header_t * header, uint8_t tag, uint8_t N, uint8_t K, uint32_t block_id, uint8_t block_index, uint16_t payload_size) {
    header->tag = tag;
    header->N = N;
    header->K = K;
    nt_fec_header_set_block_id(header, block_id);
    header->block_index = block_index;
    header->payload_size_n = htons(payload_size);
}

static error_t nt_dummy_fec_encode(nt_fec_t * fec, uint8_t *input, int size, void * extra, nt_fec_on_encode_t callback) {
    nt_fec_header_t header;
    nt_fec_header_init(&header, NT_DUMMY_FEC_TAG, 0, 0, 0, 0, (uint16_t) size);
    return callback(extra, &header, input, size);
}

static error_t nt_dummy_fec_decode(nt_fec_t * fec, nt_fec_header_t * header, uint8_t *input, int size, void * extra, nt_fec_on_decode_t callback) {
    if (header->tag != NT_DUMMY_FEC_TAG || header->N != 0 || header->K != 0) {
        return E_FEC_INVALID_HEADER;
    }

    return callback(extra, input, size);
}

void nt_dummy_fec_init(nt_fec_t * fec) {
    nt_fec_init(fec, nt_dummy_fec_encode, nt_dummy_fec_decode);
}
