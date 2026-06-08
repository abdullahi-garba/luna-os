/* include/types.h
 * Luna OS — Project AETERNA
 * Freestanding primitive type definitions.
 * No stdlib dependency. Replaces stdint.h, stdbool.h, stddef.h entirely.
 * All kernel and driver code includes this first.
 */

#ifndef LUNA_TYPES_H
#define LUNA_TYPES_H

/* ── Unsigned integers ─────────────────────────────────────── */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

/* ── Signed integers ───────────────────────────────────────── */
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

/* ── Pointer-width types ────────────────────────────────────── */
#ifdef ARCH_X86
typedef uint32_t  uintptr_t;
typedef int32_t   intptr_t;
typedef uint32_t  size_t;
typedef int32_t   ssize_t;
#elif defined(ARCH_ARM64) || defined(ARCH_X86_64)
typedef uint64_t  uintptr_t;
typedef int64_t   intptr_t;
typedef uint64_t  size_t;
typedef int64_t   ssize_t;
#endif

/* ── Boolean ────────────────────────────────────────────────── */
typedef uint8_t   bool;
#define true      1
#define false     0

/* ── NULL ───────────────────────────────────────────────────── */
#define NULL      ((void*)0)

/* ── Limits ─────────────────────────────────────────────────── */
#define UINT8_MAX   0xFF
#define UINT16_MAX  0xFFFF
#define UINT32_MAX  0xFFFFFFFFU
#define UINT64_MAX  0xFFFFFFFFFFFFFFFFULL
#define INT32_MAX   0x7FFFFFFF
#define INT32_MIN   (-0x7FFFFFFF - 1)

/* ── Common attribute macros ────────────────────────────────── */
#define PACKED      __attribute__((packed))
#define NORETURN    __attribute__((noreturn))
#define ALIGNED(n)  __attribute__((aligned(n)))
#define UNUSED      __attribute__((unused))
#define NOINLINE    __attribute__((noinline))
#define ALWAYS_INLINE __attribute__((always_inline)) inline

/* ── Compile-time assertion ─────────────────────────────────── */
#define STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)

STATIC_ASSERT(sizeof(uint8_t)  == 1, "uint8_t size mismatch");
STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t size mismatch");
STATIC_ASSERT(sizeof(uint64_t) == 8, "uint64_t size mismatch");

#endif /* LUNA_TYPES_H */
