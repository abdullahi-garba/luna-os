/* kernel/arena.h */
#ifndef LUNA_ARENA_H
#define LUNA_ARENA_H

#include "../include/types.h"

/* Pool sizes — tunable based on available RAM */
#define ARENA_LANG_SIZE  (256 * 1024)  /* 256KB for scripting language AST */
#define ARENA_AI_SIZE    (512 * 1024)  /* 512KB for AI inference context */
#define ARENA_TMP_SIZE   (128 * 1024)  /* 128KB for transient allocations */

typedef struct {
    uint8_t*    pool;
    size_t      capacity;
    size_t      cursor;
    size_t      high_watermark;
    const char* name;
    uint32_t    alloc_count;
    uint32_t    oom_count;
} Arena;

typedef struct {
    size_t   cursor;
    uint32_t alloc_count;
} ArenaCheckpoint;

/* Global arena instances */
extern Arena g_lang_arena;
extern Arena g_ai_arena;
extern Arena g_tmp_arena;

void  arena_global_init(void);
void  arena_init(Arena* a, uint8_t* pool, size_t size, const char* name);

/* Allocation */
void* arena_alloc(Arena* a, size_t size);
void* arena_calloc(Arena* a, size_t count, size_t elem_size);
char* arena_strdup(Arena* a, const char* str);

/* Reset */
void arena_reset(Arena* a);
ArenaCheckpoint arena_save(Arena* a);
void arena_restore(Arena* a, ArenaCheckpoint cp);

/* Diagnostics */
size_t   arena_used(const Arena* a);
size_t   arena_remaining(const Arena* a);
uint32_t arena_watermark_pct(const Arena* a);

#endif /* LUNA_ARENA_H */
