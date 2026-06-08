/* kernel/ledger.h */
#ifndef LUNA_LEDGER_H
#define LUNA_LEDGER_H

#include "../include/types.h"

#define LEDGER_TARGET_LEN  64

/* Action type codes */
#define LEDGER_ACTION_GENESIS    0
#define LEDGER_ACTION_VFS_CREATE 1
#define LEDGER_ACTION_VFS_READ   2
#define LEDGER_ACTION_VFS_WRITE  3
#define LEDGER_ACTION_VFS_DELETE 4
#define LEDGER_ACTION_PROC_SPAWN 5
#define LEDGER_ACTION_PROC_KILL  6
#define LEDGER_ACTION_SYSCALL    7
#define LEDGER_ACTION_NET_SEND   8
#define LEDGER_ACTION_NET_RECV   9
#define LEDGER_ACTION_CRYPTO_OP  10
#define LEDGER_ACTION_AUTH       11

/* Stream IDs */
#define LEDGER_STREAM_KERNEL     0
#define LEDGER_STREAM_COMPOSITOR 1

/* DAG Ledger Block — 32-byte SHA-256 hash appended last */
typedef struct {
    uint32_t index;
    uint64_t timestamp;          /* system ticks */
    uint8_t  action_type;
    uint8_t  stream_id;
    uint8_t  parent_count;       /* 0 = genesis, 1 = linear, 2 = DAG merge */
    uint8_t  reserved;
    char     target[LEDGER_TARGET_LEN];
    uint8_t  prev_hash[2][32];   /* up to 2 parent SHA-256 hashes */
    /* block_hash MUST be last field — SHA-256 computed over everything before it */
    uint8_t  block_hash[32];
} LedgerBlock;

void              ledger_init(void);
void              ledger_enqueue(uint8_t action, uint8_t stream_id,
                                 const char* target, uint64_t timestamp);
void              ledger_flush_task(void);  /* call from scheduler background */
uint32_t          ledger_get_count(void);
const LedgerBlock* ledger_get_block(uint32_t idx);
bool              ledger_verify_chain(void);

#endif /* LUNA_LEDGER_H */
