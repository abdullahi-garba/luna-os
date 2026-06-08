/* kernel/vfs.h — Luna OS Virtual File System */
#ifndef LUNA_VFS_H
#define LUNA_VFS_H

#include "../include/types.h"

#define VFS_NAME_LEN 64

typedef enum {
    FS_FILE,
    FS_DIRECTORY,
    FS_CHARDEVICE,
    FS_BLOCKDEVICE,
    FS_PIPE,
    FS_MOUNTPOINT
} VFS_NodeType;

typedef struct VFS_Node {
    char     name[VFS_NAME_LEN];
    uint32_t inode;
    uint32_t size;
    uint32_t uid;
    uint32_t gid;
    uint32_t permissions;
    VFS_NodeType type;
    
    /* Function pointers for hardware/driver specific operations */
    uint32_t (*read)(struct VFS_Node*, uint32_t, uint32_t, uint8_t*);
    uint32_t (*write)(struct VFS_Node*, uint32_t, uint32_t, uint8_t*);
    void     (*open)(struct VFS_Node*);
    void     (*close)(struct VFS_Node*);
    
    struct VFS_Node* ptr; /* Symlink or mount target */
} VFS_Node;

void vfs_init(void);
uint32_t vfs_read(VFS_Node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t vfs_write(VFS_Node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void vfs_open(VFS_Node* node);
void vfs_close(VFS_Node* node);

#endif /* LUNA_VFS_H */