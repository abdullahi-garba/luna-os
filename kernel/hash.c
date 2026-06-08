/* kernel/hash.c — Luna OS freestanding hash implementations
 * Provides:
 *   - FNV-1a 64-bit (fast, non-cryptographic — for ledger node IDs)
 *   - SHA-256 (cryptographic — for ledger block chaining)
 * No stdlib, no libm, fully freestanding.
 */

#include "hash.h"
#include "../include/types.h"

/* ════════════════════════════════════════════════════════════════════════════
 * FNV-1a 64-bit
 * ════════════════════════════════════════════════════════════════════════════ */

#define FNV_OFFSET_BASIS_64 0xCBF29CE484222325ULL
#define FNV_PRIME_64        0x00000100000001B3ULL

uint64_t fnv1a_64(const uint8_t* data, size_t len) {
    uint64_t hash = FNV_OFFSET_BASIS_64;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)data[i];
        hash *= FNV_PRIME_64;
    }
    return hash;
}

uint64_t fnv1a_str(const char* str) {
    uint64_t hash = FNV_OFFSET_BASIS_64;
    while (*str) {
        hash ^= (uint64_t)(uint8_t)*str++;
        hash *= FNV_PRIME_64;
    }
    return hash;
}

/* ════════════════════════════════════════════════════════════════════════════
 * SHA-256 — Freestanding Implementation
 * FIPS 180-4 compliant. No external dependencies.
 * ════════════════════════════════════════════════════════════════════════════ */

/* SHA-256 round constants (first 32 bits of fractional parts of cube roots
 * of first 64 primes) */
static const uint32_t K[64] = {
    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U,
    0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
    0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U,
    0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
    0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
    0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
    0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
    0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
    0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U,
    0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
    0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U,
    0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
    0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U,
    0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
    0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U
};

/* Initial hash values (first 32 bits of fractional parts of square roots
 * of first 8 primes) */
static const uint32_t H0[8] = {
    0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
    0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U
};

/* Rotation macros */
#define ROTR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHR(x, n)    ((x) >> (n))

#define SHA256_CH(e, f, g)  (((e) & (f)) ^ (~(e) & (g)))
#define SHA256_MAJ(a, b, c) (((a) & (b)) ^ ((a) & (c)) ^ ((b) & (c)))
#define SHA256_EP0(a)       (ROTR32(a,2)  ^ ROTR32(a,13) ^ ROTR32(a,22))
#define SHA256_EP1(e)       (ROTR32(e,6)  ^ ROTR32(e,11) ^ ROTR32(e,25))
#define SHA256_SIG0(x)      (ROTR32(x,7)  ^ ROTR32(x,18) ^ SHR(x,3))
#define SHA256_SIG1(x)      (ROTR32(x,17) ^ ROTR32(x,19) ^ SHR(x,10))

/* Process a single 512-bit (64-byte) block */
static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;

    /* Prepare message schedule */
    for (int i = 0; i < 16; i++) {
        w[i]  = ((uint32_t)block[i*4+0] << 24)
              | ((uint32_t)block[i*4+1] << 16)
              | ((uint32_t)block[i*4+2] <<  8)
              | ((uint32_t)block[i*4+3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = SHA256_SIG1(w[i-2]) + w[i-7] + SHA256_SIG0(w[i-15]) + w[i-16];
    }

    /* Initialize working variables */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    /* 64 rounds */
    for (int i = 0; i < 64; i++) {
        t1 = h + SHA256_EP1(e) + SHA256_CH(e,f,g) + K[i] + w[i];
        t2 = SHA256_EP0(a) + SHA256_MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    /* Add compressed chunk to current hash value */
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    uint32_t state[8];
    uint8_t  block[64];
    uint64_t bit_len = (uint64_t)len * 8;

    /* Initialize hash state */
    for (int i = 0; i < 8; i++) state[i] = H0[i];

    /* Process full 64-byte blocks */
    size_t remaining = len;
    while (remaining >= 64) {
        sha256_transform(state, data);
        data      += 64;
        remaining -= 64;
    }

    /* Build final padded block(s) */
    /* Copy remaining bytes */
    for (size_t i = 0; i < remaining; i++) block[i] = data[i];
    /* Append bit '1' (0x80 byte) */
    block[remaining++] = 0x80;

    /* If we don't have room for the 8-byte length, flush this block first */
    if (remaining > 56) {
        while (remaining < 64) block[remaining++] = 0;
        sha256_transform(state, block);
        remaining = 0;
    }

    /* Pad with zeros to byte 56 */
    while (remaining < 56) block[remaining++] = 0;

    /* Append original length in bits as big-endian 64-bit */
    block[56] = (uint8_t)(bit_len >> 56);
    block[57] = (uint8_t)(bit_len >> 48);
    block[58] = (uint8_t)(bit_len >> 40);
    block[59] = (uint8_t)(bit_len >> 32);
    block[60] = (uint8_t)(bit_len >> 24);
    block[61] = (uint8_t)(bit_len >> 16);
    block[62] = (uint8_t)(bit_len >>  8);
    block[63] = (uint8_t)(bit_len >>  0);
    sha256_transform(state, block);

    /* Produce final hash in big-endian byte order */
    for (int i = 0; i < 8; i++) {
        out[i*4+0] = (uint8_t)(state[i] >> 24);
        out[i*4+1] = (uint8_t)(state[i] >> 16);
        out[i*4+2] = (uint8_t)(state[i] >>  8);
        out[i*4+3] = (uint8_t)(state[i] >>  0);
    }
}

/* Utility: print SHA-256 digest as hex string into buf (must be ≥65 bytes) */
static const char HEX_TABLE[] = "0123456789abcdef";

void sha256_to_hex(const uint8_t digest[32], char* buf) {
    for (int i = 0; i < 32; i++) {
        buf[i*2+0] = HEX_TABLE[(digest[i] >> 4) & 0xF];
        buf[i*2+1] = HEX_TABLE[(digest[i]     ) & 0xF];
    }
    buf[64] = '\0';
}
