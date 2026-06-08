/* kernel/vfs.c — Luna OS Virtual File System
 * Abstract node routing that natively logs all actions to the DAG-Ledger.
 */
#include "vfs.h"
#include "string.h"
#include "ledger.h"
#include "ledger_queue.h"
#include "hash.h"

/* The root of the filesystem */
VFS_Node* fs_root = NULL;

void vfs_init(void) {
    fs_root = NULL; /* To be populated by ramdisk/storage driver later */
}

uint32_t vfs_read(VFS_Node* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node && node->read) {
        uint32_t ret = node->read(node, offset, size, buffer);
        
        /* Push forensic event to DAG-Ledger */
        LedgerEvent ev;
        luna_memset(&ev, 0, sizeof(LedgerEvent));
        ev.action_type = LEDGER_ACTION_VFS_READ;
        ev.stream_id   = LEDGER_STREAM_KERNEL;
        ev.vfs_node_id = node->inode;
        luna_strncpy(ev.target, node->name, LEDGER_TARGET_LEN - 1);
        ledger_queue_push(&ev);
        
        return ret;
    }
    return 0;
}

uint32_t vfs_write(VFS_Node* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node && node->write) {
        uint32_t ret = node->write(node, offset, size, buffer);
        
        /* Push forensic event to DAG-Ledger */
        LedgerEvent ev;
        luna_memset(&ev, 0, sizeof(LedgerEvent));
        ev.action_type = LEDGER_ACTION_VFS_WRITE;
        ev.stream_id   = LEDGER_STREAM_KERNEL;
        ev.vfs_node_id = node->inode;
        luna_strncpy(ev.target, node->name, LEDGER_TARGET_LEN - 1);
        ledger_queue_push(&ev);
        
        return ret;
    }
    return 0;
}

void vfs_open(VFS_Node* node) {
    if (node && node->open) node->open(node);
}

void vfs_close(VFS_Node* node) {
    if (node && node->close) node->close(node);
}