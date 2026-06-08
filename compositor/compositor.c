/* compositor/compositor.c — Ring 0 GUI Compositor
 * Merges Shared Surfaces from Ring 3 apps onto the hardware Framebuffer.
 */
#include "../drivers/gfx/framebuffer.h"
#include "../ipc/shared_surface.h"
#include "../kernel/string.h"

#define MAX_WINDOWS 16

typedef struct {
    uint32_t surface_id;
    int32_t  x, y;
    uint32_t w, h;
    bool     active;
    uint32_t z_order;
} Window;

static Window windows[MAX_WINDOWS];

void compositor_init(void) {
    luna_memset(windows, 0, sizeof(windows));
}

/* Called by the timer tick or VSync interrupt to redraw the screen */
void compositor_render(void) {
    /* 1. Clear background */
    fb_clear(0x00223344); /* Tactical dark blue */

    /* 2. Paint windows from bottom z-order to top */
    for (int z = 0; z < MAX_WINDOWS; z++) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].active && windows[i].z_order == z) {
                SharedSurface* s = surface_get(windows[i].surface_id);
                if (s && s->virt_addr_kernel) {
                    /* Copy Ring 3 rendered buffer to Framebuffer */
                    for (uint32_t wy = 0; wy < s->height; wy++) {
                        for (uint32_t wx = 0; wx < s->width; wx++) {
                            uint32_t color = s->virt_addr_kernel[wy * s->width + wx];
                            /* 0xFF00FF is our transparent key */
                            if (color != 0x00FF00FF) {
                                fb_put_pixel(windows[i].x + wx, windows[i].y + wy, color);
                            }
                        }
                    }
                }
            }
        }
    }
}