/**
 * @author caofuxiang
 *         2015-12-30 20:13:13.
 */

#ifndef NETTLE_KA_ENCRYPTOR_H
#define NETTLE_KA_ENCRYPTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void * internal;
} ka_encryptor_t;

typedef enum {
    KA_COMPRESSION_NONE = 0,
    KA_COMPRESSION_SNAPPY = 1
} ka_compression_type_t;

void ka_encryptor_init(ka_encryptor_t * encryptor, const char * host, int port, const char * sid, ka_compression_type_t compression_type);

bool ka_encryptor_start(ka_encryptor_t * encryptor);

bool ka_encryptor_stop(ka_encryptor_t * encryptor);

const char *ka_encryptor_encrypt(ka_encryptor_t *encryptor, const char *raw, size_t input_size, size_t *output_size);

const char *ka_encryptor_decrypt(ka_encryptor_t *encryptor, const char *cipher, size_t input_size, size_t *output_size);

void ka_encryptor_destroy(ka_encryptor_t * encryptor);

#ifdef __cplusplus
}
#endif
#endif //NETTLE_KA_ENCRYPTOR_H
