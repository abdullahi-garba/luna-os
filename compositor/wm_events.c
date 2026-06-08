/* compositor/wm_events.c — Input Event Router */
#include "../include/types.h"
#include "../ipc/shared_surface.h"

void wm_inject_mouse(int32_t dx, int32_t dy, uint8_t buttons) {
    /* 1. Update cursor X/Y */
    /* 2. Check if cursor is over a window surface */
    /* 3. If yes, send mouse event to the owner process via message queue */
}