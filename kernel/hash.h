/* kernel/hash.h */
#ifndef LUNA_HASH_H
#define LUNA_HASH_H

#include "../include/types.h"

/* FNV-1a 64-bit (fast non-cryptographic hash) */
uint64_t fnv1a_64(const uint8_t* data, size_t len);
uint64_t fnv1a_str(const char* str);

/* SHA-256 (cryptographic — FIPS 180-4) */
void sha256(const uint8_t* data, size_t len, uint8_t out[32]);
void sha256_to_hex(const uint8_t digest[32], char* hex_buf_65);

#endif /* LUNA_HASH_H */
