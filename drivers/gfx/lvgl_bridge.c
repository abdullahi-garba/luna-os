/* drivers/gfx/lvgl_bridge.c — LVGL Hardware Abstraction */
#include "framebuffer.h"
#include "../../include/types.h"

/* LVGL calls this to flush its buffer to the screen */
void luna_lvgl_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint32_t* color_p) {
    for (int32_t y = y1; y <= y2; y++) {
        for (int32_t x = x1; x <= x2; x++) {
            fb_put_pixel(x, y, *color_p++);
        }
    }
    /* Notify LVGL that flushing is complete */
    extern void lv_display_flush_ready(void*);
    lv_display_flush_ready(NULL);
}