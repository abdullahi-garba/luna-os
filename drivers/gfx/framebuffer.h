/* drivers/gfx/framebuffer.h — Raw Linear Framebuffer */
#ifndef LUNA_FRAMEBUFFER_H
#define LUNA_FRAMEBUFFER_H

#include "../../include/types.h"

void fb_init(uint32_t addr, uint32_t pitch, uint32_t w, uint32_t h, uint8_t bpp);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_clear(uint32_t color);

uint32_t fb_get_width(void);
uint32_t fb_get_height(void);

#endif /* LUNA_FRAMEBUFFER_H */