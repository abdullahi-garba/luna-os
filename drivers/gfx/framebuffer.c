/* drivers/gfx/framebuffer.c — Bare-metal Multiboot2 pixel pushing */
#include "framebuffer.h"
#include "../../kernel/vmm.h"
#include "../../kernel/string.h"

static uint32_t* fb_ptr = NULL;
static uint32_t  fb_pitch, fb_width, fb_height;
static uint8_t   fb_bpp;

void fb_init(uint32_t addr, uint32_t pitch, uint32_t w, uint32_t h, uint8_t bpp) {
    /* * The framebuffer physical address must be identity-mapped or mapped 
     * into the higher half so the kernel can write to it. 
     * Multiboot2 gives us the physical address.
     */
    fb_ptr    = (uint32_t*)addr; /* Assume identity mapped by bootloader for now */
    fb_pitch  = pitch;
    fb_width  = w;
    fb_height = h;
    fb_bpp    = bpp;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height || !fb_ptr) return;
    /* pitch is in bytes, so divide by 4 for 32-bit pixel array index */
    uint32_t offset = (y * fb_pitch) / 4 + x;
    fb_ptr[offset] = color;
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t cy = y; cy < y + h; cy++) {
        for (uint32_t cx = x; cx < x + w; cx++) {
            fb_put_pixel(cx, cy, color);
        }
    }
}

void fb_clear(uint32_t color) {
    fb_fill_rect(0, 0, fb_width, fb_height, color);
}

uint32_t fb_get_width(void)  { return fb_width; }
uint32_t fb_get_height(void) { return fb_height; }