/* kernel/pmm.c — Luna OS Physical Memory Manager
 * Bitmap allocator over the multiboot2 memory map.
 * Each bit represents one 4KB physical frame.
 * Frame 0 = physical address 0x000000, Frame 1 = 0x001000, etc.
 */
#include "pmm.h"
#include "string.h"
#include "../include/types.h"
#include "../include/panic.h"
#include "../kernel/multiboot2.h"

#define FRAME_SIZE    4096
#define FRAMES_TOTAL  (1024 * 1024)  /* supports up to 4GB physical RAM */
#define BITMAP_WORDS  (FRAMES_TOTAL / 32)

/* ── Bitmap ──────────────────────────────────────────────────────────────── */
/* 1 = used/reserved, 0 = free */
static uint32_t bitmap[BITMAP_WORDS];
static uint32_t total_frames  = 0;
static uint32_t free_frames   = 0;
static uint32_t last_alloc    = 0;   /* next-fit hint */

static ALWAYS_INLINE void frame_set(uint32_t frame) {
    bitmap[frame / 32] |= (1U << (frame % 32));
}
static ALWAYS_INLINE void frame_clear(uint32_t frame) {
    bitmap[frame / 32] &= ~(1U << (frame % 32));
}
static ALWAYS_INLINE bool frame_test(uint32_t frame) {
    return (bitmap[frame / 32] >> (frame % 32)) & 1;
}

/* ── Kernel physical extent (set from linker symbols) ────────────────────── */
extern uint32_t __kernel_end_phys;   /* from linker.ld */
#define KERNEL_PHYS_BASE 0x00100000U

/* ── Init ────────────────────────────────────────────────────────────────── */
void pmm_init(MB2_MmapEntry* entries, uint32_t count) {
    /* Start: mark everything as used */
    luna_memset(bitmap, 0xFF, sizeof(bitmap));
    free_frames = 0;

    /* Walk mmap — free available regions */
    for (uint32_t i = 0; i < count; i++) {
        if (entries[i].type != MB2_MMAP_AVAILABLE) continue;

        uint64_t base = entries[i].base_addr;
        uint64_t len  = entries[i].length;

        /* Skip the first 1MB (BIOS/VGA/legacy) */
        if (base < 0x100000) {
            uint64_t skip = 0x100000 - base;
            if (skip >= len) continue;
            base += skip;
            len  -= skip;
        }

        uint32_t frame_start = (uint32_t)(base / FRAME_SIZE);
        uint32_t frame_count = (uint32_t)(len  / FRAME_SIZE);

        for (uint32_t f = frame_start; f < frame_start + frame_count; f++) {
            if (f >= FRAMES_TOTAL) break;
            frame_clear(f);
            free_frames++;
            total_frames++;
        }
    }

    /* Re-mark kernel frames as used (physical 1MB to kernel_end_phys) */
    uint32_t k_end_frame = ((uint32_t)&__kernel_end_phys + FRAME_SIZE - 1) / FRAME_SIZE;
    for (uint32_t f = KERNEL_PHYS_BASE / FRAME_SIZE; f <= k_end_frame; f++) {
        if (!frame_test(f)) { frame_set(f); free_frames--; }
    }
}

/* ── Allocate one frame (next-fit) ──────────────────────────────────────── */
uint32_t pmm_alloc_frame(void) {
    if (free_frames == 0) kernel_panic("PMM: Out of physical memory");

    /* Search from last allocation point (next-fit reduces fragmentation) */
    for (uint32_t i = 0; i < FRAMES_TOTAL; i++) {
        uint32_t f = (last_alloc + i) % FRAMES_TOTAL;
        if (!frame_test(f)) {
            frame_set(f);
            free_frames--;
            last_alloc = f + 1;
            return f * FRAME_SIZE;  /* return physical address */
        }
    }

    kernel_panic("PMM: Bitmap inconsistent");
    return 0;
}

/* ── Allocate N contiguous frames ────────────────────────────────────────── */
uint32_t pmm_alloc_frames(uint32_t n) {
    uint32_t start = 0, run = 0;
    for (uint32_t f = 0; f < FRAMES_TOTAL; f++) {
        if (!frame_test(f)) {
            if (run == 0) start = f;
            if (++run == n) {
                for (uint32_t k = start; k < start + n; k++) {
                    frame_set(k);
                    free_frames--;
                }
                return start * FRAME_SIZE;
            }
        } else {
            run = 0;
        }
    }
    kernel_panic("PMM: No contiguous frames");
    return 0;
}

/* ── Free frame ──────────────────────────────────────────────────────────── */
void pmm_free_frame(uint32_t phys_addr) {
    uint32_t f = phys_addr / FRAME_SIZE;
    if (f >= FRAMES_TOTAL) return;
    if (!frame_test(f)) return;  /* double-free guard */
    frame_clear(f);
    free_frames++;
}

/* ── Stats ───────────────────────────────────────────────────────────────── */
uint32_t pmm_free_frames(void)  { return free_frames; }
uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_free_kb(void)      { return (free_frames * FRAME_SIZE) / 1024; }
