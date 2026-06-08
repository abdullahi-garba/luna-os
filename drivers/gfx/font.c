/* drivers/gfx/font.c — PC Screen Font (PSF2) Renderer */
#include "font.h"
#include "framebuffer.h"

#define PSF2_MAGIC 0x864ab572

typedef struct PACKED {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t length;
    uint32_t char_size;
    uint32_t height;
    uint32_t width;
} PSF2_Header;

static PSF2_Header* font_hdr = NULL;

void font_init(void* psf2_binary) {
    PSF2_Header* hdr = (PSF2_Header*)psf2_binary;
    if (hdr->magic == PSF2_MAGIC) {
        font_hdr = hdr;
    }
}

void font_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    if (!font_hdr) return;

    /* Get pointer to the glyph data */
    uint8_t* glyph = (uint8_t*)font_hdr + font_hdr->header_size + (c * font_hdr->char_size);
    uint32_t pitch = (font_hdr->width + 7) / 8;

    for (uint32_t cy = 0; cy < font_hdr->height; cy++) {
        for (uint32_t cx = 0; cx < font_hdr->width; cx++) {
            /* Check if the bit for this pixel is set */
            if ((glyph[cy * pitch + (cx / 8)] >> (7 - (cx % 8))) & 1) {
                fb_put_pixel(x + cx, y + cy, fg_color);
            } else if (bg_color != 0x00000000) { /* 0 = transparent background */
                fb_put_pixel(x + cx, y + cy, bg_color);
            }
        }
    }
}

void font_draw_string(uint32_t x, uint32_t y, const char* str, uint32_t fg_color, uint32_t bg_color) {
    uint32_t cursor_x = x;
    while (*str) {
        font_draw_char(cursor_x, y, *str, fg_color, bg_color);
        cursor_x += (font_hdr ? font_hdr->width : 8);
        str++;
    }
}