#include <stddef.h>
#include "bootinfo.h"
#include "framebuffer.h"
#include "font8x8.h"

void fb_init(Framebuffer *fb, const struct BootInfo *info) {
    fb->addr = (uint32_t *)(uintptr_t)info->lfb;
    fb->back_buffer = NULL;  /* No double buffering by default */
    fb->width = info->width;
    fb->height = info->height;
    fb->pitch = info->pitch;
    fb->bpp = info->bpp;
    fb->font = (const uint8_t *)(uintptr_t)info->font_ptr;
}

void fb_enable_double_buffer(Framebuffer *fb, uint32_t *buffer) {
    fb->back_buffer = buffer;
}

void fb_swap(Framebuffer *fb) {
    uint16_t y;
    
    if (!fb->back_buffer) {
        return;  /* No double buffering enabled */
    }
    
    /* Copy back buffer to visible framebuffer */
    for (y = 0; y < fb->height; ++y) {
        uint32_t *dst = (uint32_t *)((uint8_t *)fb->addr + (y * fb->pitch));
        uint32_t *src = (uint32_t *)((uint8_t *)fb->back_buffer + (y * fb->pitch));
        uint16_t x;
        
        for (x = 0; x < fb->width; ++x) {
            dst[x] = src[x];
        }
    }
}

void fb_clear(Framebuffer *fb, uint32_t color) {
    uint16_t y;
    uint32_t *target = fb->back_buffer ? fb->back_buffer : fb->addr;

    for (y = 0; y < fb->height; ++y) {
        uint32_t *row = (uint32_t *)((uint8_t *)target + (y * fb->pitch));
        uint16_t x;

        for (x = 0; x < fb->width; ++x) {
            row[x] = color;
        }
    }
}

void fb_draw_rect(Framebuffer *fb, int x, int y, int w, int h, uint32_t color) {
    int yy;
    uint32_t *target = fb->back_buffer ? fb->back_buffer : fb->addr;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > fb->width) {
        w = fb->width - x;
    }
    if (y + h > fb->height) {
        h = fb->height - y;
    }
    if (w <= 0 || h <= 0) {
        return;
    }

    for (yy = 0; yy < h; ++yy) {
        uint32_t *row = (uint32_t *)((uint8_t *)target + ((y + yy) * fb->pitch));
        int xx;

        for (xx = 0; xx < w; ++xx) {
            row[x + xx] = color;
        }
    }
}

void fb_draw_char(Framebuffer *fb, int x, int y, char c, uint32_t color) {
    const uint8_t *glyph;
    int row, col;
    unsigned char uc = (unsigned char)c;
    uint32_t *target = fb->back_buffer ? fb->back_buffer : fb->addr;
    
    /* Use embedded font (ASCII 32-127) */
    if (uc < 32 || uc > 127) {
        uc = 32; /* Default to space for out-of-range chars */
    }
    
    /* Adjust font pointer: kernel linked at 0 but loaded at 0x20000 */
    glyph = (const uint8_t *)((uintptr_t)font8x8_basic[uc - 32] + 0x20000);
    
    for (row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        int py = y + row;
        
        if (py >= 0 && py < fb->height) {
            uint32_t *dst = (uint32_t *)((uint8_t *)target + (py * fb->pitch));
            
            for (col = 0; col < 8; col++) {
                int px = x + col;
                
                if (px >= 0 && px < fb->width) {
                    if (bits & (1 << col)) {
                        dst[px] = color;
                    }
                }
            }
        }
    }
}

void fb_draw_text(Framebuffer *fb, int x, int y, const char *text, uint32_t color) {
    int i;
    const char *actual_text;
    
    if (!text || !fb) {
        return;
    }
    
    /* Adjust pointer: kernel linked at 0 but loaded at 0x20000 */
    /* Check if pointer looks like it might already be adjusted */
    uintptr_t ptr_val = (uintptr_t)text;
    if (ptr_val < 0x10000) {
        /* Looks like an offset, needs adjustment */
        actual_text = (const char *)(ptr_val + 0x20000);
    } else {
        /* Already adjusted or in a different region */
        actual_text = text;
    }
    
    /* Safety check - bail if pointer looks invalid */
    if ((uintptr_t)actual_text < 0x20000 || (uintptr_t)actual_text > 0x100000) {
        return;
    }
    
    for (i = 0; i < 64 && actual_text[i] != '\0'; i++) {
        fb_draw_char(fb, x + (i * 8), y, actual_text[i], color);
    }
}
