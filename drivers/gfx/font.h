/* drivers/gfx/font.h — PSF2 Font Renderer */
#ifndef LUNA_FONT_H
#define LUNA_FONT_H

#include "../../include/types.h"

void font_init(void* psf2_binary);
void font_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
void font_draw_string(uint32_t x, uint32_t y, const char* str, uint32_t fg_color, uint32_t bg_color);

#endif /* LUNA_FONT_H */