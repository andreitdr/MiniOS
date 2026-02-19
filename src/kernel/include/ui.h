#ifndef UI_H
#define UI_H

#include <stdint.h>

#include "framebuffer.h"
#include "mouse.h"

typedef struct {
    int bar_height;
    int button_width;
    int button_height;
    int button_padding;
    uint32_t bg_color;
    uint32_t bar_color;
    uint32_t button_color;
    uint32_t button_hover;
    uint32_t text_color;
    uint32_t accent_color;
} UITheme;

typedef struct {
    int info_x;
    int info_y;
    int info_w;
    int info_h;
    int halt_x;
    int halt_y;
    int halt_w;
    int halt_h;
} UIButtonLayout;

void ui_theme_default(UITheme *theme);
void ui_layout_default(UIButtonLayout *layout, const Framebuffer *fb, const UITheme *theme);
void ui_draw(Framebuffer *fb, const UITheme *theme, const UIButtonLayout *layout, const char *time_text, const MouseState *mouse);
int ui_hit_test(const UIButtonLayout *layout, int x, int y);

#endif
