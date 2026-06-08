/* ipc/shared_surface.h — Ring 0 / Ring 3 Window Sandbox Boundary */
#ifndef LUNA_SHARED_SURFACE_H
#define LUNA_SHARED_SURFACE_H

#include "../include/types.h"

#define MAX_SURFACES 32

/* * A surface is a chunk of physical memory mapped into BOTH the kernel's
 * compositor and the Ring 3 GUI application. This allows zero-copy UI rendering.
 */
typedef struct {
    uint32_t  id;
    uint32_t  owner_pid;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    uint32_t* virt_addr_kernel; /* Where Ring 0 reads the pixels */
    uint32_t* virt_addr_user;   /* Where Ring 3 writes the pixels */
    uint32_t  phys_base;
    bool      active;
} SharedSurface;

void          surface_init(void);
SharedSurface* surface_create(uint32_t pid, uint32_t w, uint32_t h);
void          surface_destroy(uint32_t id);
SharedSurface* surface_get(uint32_t id);

#endif /* LUNA_SHARED_SURFACE_H */