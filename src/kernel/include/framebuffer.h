#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

struct BootInfo;

typedef struct {
    uint32_t *addr;          /* Visible framebuffer */
    uint32_t *back_buffer;   /* Off-screen buffer for double buffering */
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint8_t bpp;
    const uint8_t *font;
} Framebuffer;

void fb_init(Framebuffer *fb, const struct BootInfo *info);
void fb_enable_double_buffer(Framebuffer *fb, uint32_t *buffer);
void fb_swap(Framebuffer *fb);
void fb_clear(Framebuffer *fb, uint32_t color);
void fb_draw_rect(Framebuffer *fb, int x, int y, int w, int h, uint32_t color);
void fb_draw_char(Framebuffer *fb, int x, int y, char c, uint32_t color);
void fb_draw_text(Framebuffer *fb, int x, int y, const char *text, uint32_t color);

#endif
