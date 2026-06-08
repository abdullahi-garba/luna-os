/* ipc/shared_surface.c — Zero-copy GUI memory mapping */
#include "shared_surface.h"
#include "../kernel/pmm.h"
#include "../kernel/vmm.h"
#include "../kernel/string.h"
#include "../kernel/process.h"

static SharedSurface surfaces[MAX_SURFACES];
static uint32_t next_surface_id = 1;

void surface_init(void) {
    luna_memset(surfaces, 0, sizeof(surfaces));
}

SharedSurface* surface_create(uint32_t pid, uint32_t w, uint32_t h) {
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (!surfaces[i].active) {
            SharedSurface* s = &surfaces[i];
            s->id        = next_surface_id++;
            s->owner_pid = pid;
            s->width     = w;
            s->height    = h;
            s->pitch     = w * 4; /* 32bpp */
            
            /* Calculate pages needed */
            uint32_t size_bytes = w * h * 4;
            uint32_t pages = (size_bytes + 4095) / 4096;
            
            /* Allocate contiguous physical RAM */
            s->phys_base = pmm_alloc_frames(pages);
            
            /* Map into Kernel space (so Compositor can read it) */
            s->virt_addr_kernel = (uint32_t*)(s->phys_base + 0xC0000000);
            
            /* The user-space virtual mapping happens during the syscall handler
             * inside the requesting process's page directory. */
            s->virt_addr_user = NULL; 
            
            s->active = true;
            return s;
        }
    }
    return NULL; /* Out of surface slots */
}

SharedSurface* surface_get(uint32_t id) {
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (surfaces[i].active && surfaces[i].id == id) {
            return &surfaces[i];
        }
    }
    return NULL;
}

void surface_destroy(uint32_t id) {
    SharedSurface* s = surface_get(id);
    if (s) {
        /* Free physical frames back to PMM */
        uint32_t size_bytes = s->width * s->height * 4;
        uint32_t pages = (size_bytes + 4095) / 4096;
        for(uint32_t p = 0; p < pages; p++) {
            pmm_free_frame(s->phys_base + (p * 4096));
        }
        s->active = false;
    }
}