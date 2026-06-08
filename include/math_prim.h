/* include/math_prim.h — Luna OS freestanding math primitives */
#ifndef LUNA_MATH_PRIM_H
#define LUNA_MATH_PRIM_H

#include "types.h"

/* These replace libgcc integer helpers for 32-bit targets. */
uint32_t __udivsi3(uint32_t num, uint32_t denom);
uint32_t __umodsi3(uint32_t num, uint32_t denom);
int32_t  __divsi3(int32_t  num, int32_t  denom);
int32_t  __modsi3(int32_t  num, int32_t  denom);

/* 64-bit helpers for 32-bit targets */
uint64_t __udivdi3(uint64_t num, uint64_t denom);
uint64_t __umoddi3(uint64_t num, uint64_t denom);

/* Utility math */
static ALWAYS_INLINE uint32_t luna_min(uint32_t a, uint32_t b) { return a < b ? a : b; }
static ALWAYS_INLINE uint32_t luna_max(uint32_t a, uint32_t b) { return a > b ? a : b; }
static ALWAYS_INLINE uint32_t luna_abs(int32_t x)              { return (uint32_t)(x < 0 ? -x : x); }
static ALWAYS_INLINE uint32_t luna_align_up(uint32_t val, uint32_t align) {
    return (val + align - 1) & ~(align - 1);
}
static ALWAYS_INLINE bool luna_is_power_of_two(uint32_t n) {
    return n && !(n & (n - 1));
}

/* Bit manipulation */
static ALWAYS_INLINE uint32_t popcount32(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555U);
    x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
    return (((x + (x >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
}
static ALWAYS_INLINE uint32_t clz32(uint32_t x) {
    if (!x) return 32;
    uint32_t n = 0;
    if (!(x & 0xFFFF0000U)) { n += 16; x <<= 16; }
    if (!(x & 0xFF000000U)) { n +=  8; x <<=  8; }
    if (!(x & 0xF0000000U)) { n +=  4; x <<=  4; }
    if (!(x & 0xC0000000U)) { n +=  2; x <<=  2; }
    if (!(x & 0x80000000U)) { n +=  1; }
    return n;
}

#endif /* LUNA_MATH_PRIM_H */
