/* kernel/arena.c — Luna OS Arena Allocator
 *
 * An arena is a bump-pointer allocator over a fixed memory region.
 * Allocation: O(1) pointer increment
 * Free: O(1) cursor reset (frees everything at once)
 *
 * Used for:
 *   - Scripting language AST nodes (reset after each script completes)
 *   - AI query context (reset after each inference)
 *   - NLP intent classification (reset after each parse)
 *   - Any transient allocation that lives for a bounded scope
 *
 * Multiple arenas can coexist; each is a distinct memory region.
 */

#include "arena.h"
#include "string.h"
#include "../include/types.h"
#include "../include/panic.h"

/* ── Global arenas ───────────────────────────────────────────────────────── */
/* One per major subsystem to avoid cross-contamination */
static uint8_t lang_arena_pool[ARENA_LANG_SIZE]   ALIGNED(16);
static uint8_t ai_arena_pool  [ARENA_AI_SIZE]     ALIGNED(16);
static uint8_t tmp_arena_pool [ARENA_TMP_SIZE]    ALIGNED(16);

Arena g_lang_arena;   /* scripting language interpreter */
Arena g_ai_arena;     /* AI inference context */
Arena g_tmp_arena;    /* general transient allocations */

/* ── Init ────────────────────────────────────────────────────────────────── */
void arena_global_init(void) {
    arena_init(&g_lang_arena, lang_arena_pool, ARENA_LANG_SIZE, "lang");
    arena_init(&g_ai_arena,   ai_arena_pool,   ARENA_AI_SIZE,   "ai");
    arena_init(&g_tmp_arena,  tmp_arena_pool,  ARENA_TMP_SIZE,  "tmp");
}

void arena_init(Arena* a, uint8_t* pool, size_t size, const char* name) {
    a->pool           = pool;
    a->capacity       = size;
    a->cursor         = 0;
    a->high_watermark = 0;
    a->name           = name;
    a->alloc_count    = 0;
    a->oom_count      = 0;
    luna_memset(pool, 0, size);
}

/* ── Allocate ────────────────────────────────────────────────────────────── */
void* arena_alloc(Arena* a, size_t size) {
    /* Align to 8 bytes (sufficient for all primitive types + pointers) */
    size_t aligned = (size + 7) & ~(size_t)7;

    if (a->cursor + aligned > a->capacity) {
        a->oom_count++;
        return NULL;  /* caller must handle OOM — no panic, no blocking */
    }

    void* ptr     = (void*)(a->pool + a->cursor);
    a->cursor    += aligned;
    a->alloc_count++;

    /* Track high-water mark for profiling */
    if (a->cursor > a->high_watermark) {
        a->high_watermark = a->cursor;
    }

    return ptr;
}

/* Allocate and zero-initialize */
void* arena_calloc(Arena* a, size_t count, size_t elem_size) {
    size_t total = count * elem_size;
    void*  ptr   = arena_alloc(a, total);
    if (ptr) luna_memset(ptr, 0, total);
    return ptr;
}

/* Copy a string into the arena (returns null-terminated copy) */
char* arena_strdup(Arena* a, const char* str) {
    size_t len = luna_strlen(str);
    char*  buf = (char*)arena_alloc(a, len + 1);
    if (!buf) return NULL;
    luna_memcpy(buf, str, len);
    buf[len] = '\0';
    return buf;
}

/* ── Free / Reset ────────────────────────────────────────────────────────── */
/* Reset entire arena — O(1), frees all allocations in one operation */
void arena_reset(Arena* a) {
    a->cursor      = 0;
    a->alloc_count = 0;
    /* high_watermark intentionally preserved for diagnostics */
}

/* Save/restore for nested scopes (like function call frames) */
ArenaCheckpoint arena_save(Arena* a) {
    ArenaCheckpoint cp;
    cp.cursor      = a->cursor;
    cp.alloc_count = a->alloc_count;
    return cp;
}

void arena_restore(Arena* a, ArenaCheckpoint cp) {
    a->cursor      = cp.cursor;
    a->alloc_count = cp.alloc_count;
}

/* ── Diagnostics ─────────────────────────────────────────────────────────── */
size_t arena_used(const Arena* a) {
    return a->cursor;
}

size_t arena_remaining(const Arena* a) {
    return a->capacity - a->cursor;
}

uint32_t arena_watermark_pct(const Arena* a) {
    if (a->capacity == 0) return 0;
    return (uint32_t)(((uint64_t)a->high_watermark * 100) / a->capacity);
}
