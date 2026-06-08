/* kernel/ledger_queue.h */
#ifndef LUNA_LEDGER_QUEUE_H
#define LUNA_LEDGER_QUEUE_H

#include "../include/types.h"

/* Must be power of 2 */
#define LEDGER_QUEUE_SIZE  1024
#define LEDGER_TARGET_LEN  64

/* Lightweight event — pushed by VFS on hot path, no blocking hash */
typedef struct {
    uint8_t  action_type;
    uint8_t  stream_id;
    uint16_t reserved;
    uint64_t timestamp;
    uint32_t vfs_node_id;
    uint64_t partial_hash;      /* FNV-1a of target — for fast dedup */
    char     target[LEDGER_TARGET_LEN];
} LedgerEvent;

void     ledger_queue_init(void);
bool     ledger_queue_push(const LedgerEvent* ev);
bool     ledger_queue_pop(LedgerEvent* ev);
uint32_t ledger_queue_depth(void);

#endif /* LUNA_LEDGER_QUEUE_H */
