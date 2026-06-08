/* kernel/ledger.c — Luna OS DAG-Ledger Forensic Chain
 *
 * Design:
 *   - Every VFS/syscall action enqueues a LedgerEvent into a lock-free ring
 *     buffer (O(1) enqueue, never blocks the caller)
 *   - A background scheduler task drains the ring and builds SHA-256 chained
 *     blocks asynchronously
 *   - Two parent pointers per block (true DAG) enable concurrent write streams
 *     from shell and compositor to merge without serialization locks
 *
 * Block chain: Block[n].prev_hash[0] = SHA-256(Block[n-1])
 *              Block[n].prev_hash[1] = SHA-256(Block[last_other_stream])
 */

#include "ledger.h"
#include "ledger_queue.h"
#include "hash.h"
#include "string.h"
#include "../include/types.h"
#include "../include/panic.h"

/* ── Storage ─────────────────────────────────────────────────────────────── */
#define LEDGER_MAX_BLOCKS 4096

static LedgerBlock ledger_blocks[LEDGER_MAX_BLOCKS];
static uint32_t    ledger_count    = 0;
static uint32_t    ledger_head_idx = 0;  /* index of most recent finalized block */

/* Two stream heads for DAG merging (shell stream + compositor stream) */
static uint32_t    stream_head[2] = {0, 0};

/* ── Genesis block ───────────────────────────────────────────────────────── */
static void ledger_genesis(void) {
    LedgerBlock* b = &ledger_blocks[0];

    luna_memset(b, 0, sizeof(LedgerBlock));
    b->index        = 0;
    b->timestamp    = 0;
    b->action_type  = LEDGER_ACTION_GENESIS;
    b->stream_id    = 0;
    b->parent_count = 0;

    /* Target = "LUNA_OS_GENESIS" */
    luna_strncpy(b->target, "LUNA_OS_GENESIS", LEDGER_TARGET_LEN - 1);

    /* Hash genesis block itself */
    sha256((uint8_t*)b, sizeof(LedgerBlock) - 32, b->block_hash);

    ledger_count    = 1;
    ledger_head_idx = 0;
    stream_head[0]  = 0;
    stream_head[1]  = 0;
}

/* ── Init ────────────────────────────────────────────────────────────────── */
void ledger_init(void) {
    ledger_queue_init();
    ledger_genesis();
}

/* ── Fast-path: enqueue event (called on VFS hot path) ─────────────────── */
void ledger_enqueue(uint8_t action, uint8_t stream_id,
                    const char* target, uint64_t timestamp) {
    LedgerEvent ev;
    ev.action_type = action;
    ev.stream_id   = stream_id;
    ev.timestamp   = timestamp;
    ev.vfs_node_id = fnv1a_str(target) & 0xFFFFFFFF;

    luna_strncpy(ev.target, target, LEDGER_TARGET_LEN - 1);
    ev.target[LEDGER_TARGET_LEN - 1] = '\0';

    /* Pre-compute a quick FNV hash of the target for fast dedup */
    ev.partial_hash = fnv1a_str(target);

    ledger_queue_push(&ev);
}

/* ── Background task: drain queue and finalize blocks ──────────────────── */
/* Called from scheduler background task every 10ms */
void ledger_flush_task(void) {
    LedgerEvent ev;

    while (ledger_queue_pop(&ev)) {
        if (ledger_count >= LEDGER_MAX_BLOCKS) {
            /* Ring overflow — oldest block recycled (forensic limitation noted) */
            /* In production: flush to ATA disk via disk_image.c */
            ledger_count = 1; /* keep genesis */
        }

        LedgerBlock* b   = &ledger_blocks[ledger_count];
        uint8_t      sid = ev.stream_id & 1;

        luna_memset(b, 0, sizeof(LedgerBlock));
        b->index        = ledger_count;
        b->timestamp    = ev.timestamp;
        b->action_type  = ev.action_type;
        b->stream_id    = ev.stream_id;

        luna_strncpy(b->target, ev.target, LEDGER_TARGET_LEN - 1);

        /* Set parent hashes — true DAG: up to 2 parents */
        b->parent_count = 1;
        luna_memcpy(b->prev_hash[0],
                    ledger_blocks[stream_head[sid]].block_hash, 32);

        /* If both streams have diverged, merge them */
        if (stream_head[0] != stream_head[1]) {
            b->parent_count = 2;
            uint8_t other = sid ^ 1;
            luna_memcpy(b->prev_hash[1],
                        ledger_blocks[stream_head[other]].block_hash, 32);
        }

        /* Compute this block's SHA-256 (over everything except block_hash) */
        sha256((uint8_t*)b, sizeof(LedgerBlock) - 32, b->block_hash);

        stream_head[sid] = ledger_count;
        ledger_head_idx  = ledger_count;
        ledger_count++;
    }
}

/* ── Read / dump ─────────────────────────────────────────────────────────── */
uint32_t ledger_get_count(void) {
    return ledger_count;
}

const LedgerBlock* ledger_get_block(uint32_t idx) {
    if (idx >= ledger_count) return NULL;
    return &ledger_blocks[idx];
}

/* Verify chain integrity — walk from head back to genesis */
bool ledger_verify_chain(void) {
    uint8_t recomputed[32];

    for (uint32_t i = 1; i < ledger_count; i++) {
        LedgerBlock* b = &ledger_blocks[i];

        /* Recompute this block's hash */
        sha256((uint8_t*)b, sizeof(LedgerBlock) - 32, recomputed);

        /* Compare against stored hash */
        if (luna_memcmp(recomputed, b->block_hash, 32) != 0) {
            return false; /* Tamper detected */
        }

        /* Verify parent pointer 0 */
        if (b->parent_count > 0) {
            /* Find the referenced parent — linear scan (optimize with index later) */
            bool found = false;
            for (uint32_t j = 0; j < i; j++) {
                if (luna_memcmp(ledger_blocks[j].block_hash,
                                b->prev_hash[0], 32) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
    }
    return true;
}
