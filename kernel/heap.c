/* kernel/heap.c — Luna OS Kernel Slab Allocator
 * Fixed-size slab caches for the most common allocation sizes.
 * Falls back to direct page allocation for large objects.
 * Provides k_malloc() and k_free() for all kernel C++ and C subsystems.
 */
#include "heap.h"
#include "pmm.h"
#include "string.h"
#include "../include/types.h"
#include "../include/panic.h"

/* ── Slab configuration ──────────────────────────────────────────────────── */
/* Sizes must be powers of 2 for fast alignment */
#define NUM_SLABS      8
static const uint32_t SLAB_SIZES[NUM_SLABS] = {8, 16, 32, 64, 128, 256, 512, 1024};
#define SLAB_PAGES     4    /* pages per slab = 16KB per size class */
#define PAGE_SIZE      4096

/* ── Free list node ──────────────────────────────────────────────────────── */
typedef struct FreeNode {
    struct FreeNode* next;
} FreeNode;

/* ── Slab cache ──────────────────────────────────────────────────────────── */
typedef struct {
    FreeNode* free_list;
    uint32_t  obj_size;
    uint32_t  total;
    uint32_t  used;
} SlabCache;

static SlabCache caches[NUM_SLABS];

/* ── Large allocation header ─────────────────────────────────────────────── */
typedef struct {
    uint32_t magic;      /* 0xDEADBEEF */
    uint32_t size;       /* total bytes including header */
} LargeHdr;
#define LARGE_MAGIC 0xDEADBEEFU

/* ── Kernel heap virtual base ─────────────────────────────────────────────── */
/* Placed above kernel at 0xD0000000 — grows upward */
#define HEAP_VIRT_BASE  0xD0000000U
static uint32_t heap_cursor = HEAP_VIRT_BASE;

static uint32_t heap_alloc_pages(uint32_t n) {
    uint32_t vaddr = heap_cursor;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t phys = pmm_alloc_frame();
        /* Map into kernel virtual space using vmm */
        extern void vmm_map_page(uint32_t*, uint32_t, uint32_t, uint32_t);
        extern uint32_t* vmm_get_kernel_pd(void);
        vmm_map_page(vmm_get_kernel_pd(), heap_cursor, phys,
                     0x003 /* PRESENT | WRITE */);
        heap_cursor += PAGE_SIZE;
    }
    return vaddr;
}

/* ── Init ────────────────────────────────────────────────────────────────── */
void heap_init(void) {
    for (int i = 0; i < NUM_SLABS; i++) {
        uint32_t obj_size = SLAB_SIZES[i];
        uint32_t base     = heap_alloc_pages(SLAB_PAGES);
        uint32_t capacity = (SLAB_PAGES * PAGE_SIZE);
        uint32_t count    = capacity / obj_size;

        caches[i].obj_size  = obj_size;
        caches[i].total     = count;
        caches[i].used      = 0;
        caches[i].free_list = NULL;

        /* Thread all objects onto the free list */
        uint8_t* ptr = (uint8_t*)base;
        for (uint32_t j = 0; j < count; j++) {
            FreeNode* node  = (FreeNode*)ptr;
            node->next      = caches[i].free_list;
            caches[i].free_list = node;
            ptr += obj_size;
        }
    }
}

/* ── k_malloc ────────────────────────────────────────────────────────────── */
void* k_malloc(size_t size) {
    if (size == 0) return NULL;

    /* Find the smallest slab that fits */
    for (int i = 0; i < NUM_SLABS; i++) {
        if (size <= caches[i].obj_size) {
            if (!caches[i].free_list) {
                /* Slab exhausted — extend with more pages */
                uint32_t base  = heap_alloc_pages(SLAB_PAGES);
                uint32_t obj   = caches[i].obj_size;
                uint8_t* ptr   = (uint8_t*)base;
                uint32_t count = (SLAB_PAGES * PAGE_SIZE) / obj;
                for (uint32_t j = 0; j < count; j++) {
                    FreeNode* n = (FreeNode*)ptr;
                    n->next = caches[i].free_list;
                    caches[i].free_list = n;
                    ptr += obj;
                }
                caches[i].total += count;
            }
            FreeNode* obj   = caches[i].free_list;
            caches[i].free_list = obj->next;
            caches[i].used++;
            luna_memset(obj, 0, caches[i].obj_size);
            return (void*)obj;
        }
    }

    /* Large allocation — direct page(s) with header */
    uint32_t total  = (uint32_t)(size + sizeof(LargeHdr));
    uint32_t pages  = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t base   = heap_alloc_pages(pages);
    LargeHdr* hdr   = (LargeHdr*)base;
    hdr->magic      = LARGE_MAGIC;
    hdr->size       = pages * PAGE_SIZE;
    return (void*)(base + sizeof(LargeHdr));
}

/* ── k_free ─────────────────────────────────────────────────────────────── */
void k_free(void* ptr) {
    if (!ptr) return;

    /* Check if it's a large allocation */
    LargeHdr* hdr = (LargeHdr*)((uint8_t*)ptr - sizeof(LargeHdr));
    if (hdr->magic == LARGE_MAGIC) {
        /* Return pages to PMM — for now just mark as free in bitmap */
        /* TODO: vmm_unmap_page loop + pmm_free_frame loop */
        hdr->magic = 0;
        return;
    }

    /* Find matching slab cache by checking alignment + range */
    for (int i = 0; i < NUM_SLABS; i++) {
        /* Objects from slab are multiples of obj_size — heuristic check */
        if (((uintptr_t)ptr % caches[i].obj_size) == 0) {
            FreeNode* node = (FreeNode*)ptr;
            node->next     = caches[i].free_list;
            caches[i].free_list = node;
            caches[i].used--;
            return;
        }
    }
}

/* ── k_realloc ───────────────────────────────────────────────────────────── */
void* k_realloc(void* ptr, size_t new_size) {
    if (!ptr)   return k_malloc(new_size);
    if (!new_size) { k_free(ptr); return NULL; }
    void* new_ptr = k_malloc(new_size);
    if (new_ptr) luna_memcpy(new_ptr, ptr, new_size);
    k_free(ptr);
    return new_ptr;
}
