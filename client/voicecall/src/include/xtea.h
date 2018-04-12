
#ifndef XTEA_H
#define XTEA_H

#include <stddef.h>
#include <stdint.h>

/* For all these functions len must be multiple of 8.
 * Encryption and decryption will be performed "in place", that is,
 * overwriting the data in buf.
 */

/* electronic codebook */
void xtea_encrypt_buffer_ecb(uint8_t *buf, size_t len, const uint32_t key[4]);
void xtea_decrypt_buffer_ecb(uint8_t *buf, size_t len, const uint32_t key[4]);

/* cipher-block chaining */
void xtea_encrypt_buffer_cbc(uint8_t *buf, size_t len, const uint32_t key[4], const uint32_t iv[2]);
void xtea_decrypt_buffer_cbc(uint8_t *buf, size_t len, const uint32_t key[4], const uint32_t iv[2]);

#endif
