#include "framebuffer.h"
#include "mouse.h"
#include "ui.h"

static int point_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh));
}

void ui_theme_default(UITheme *theme) {
    theme->bar_height = 28;
    theme->button_width = 96;
    theme->button_height = 20;
    theme->button_padding = 8;
    theme->bg_color = 0x1C2433;
    theme->bar_color = 0x3B4A68;
    theme->button_color = 0x4F5F7A;
    theme->button_hover = 0x60739A;
    theme->text_color = 0xF1F4F8;
    theme->accent_color = 0xB4D5FF;
}

void ui_layout_default(UIButtonLayout *layout, const Framebuffer *fb, const UITheme *theme) {
    layout->info_x = theme->button_padding;
    layout->info_y = (theme->bar_height - theme->button_height) / 2;
    layout->info_w = theme->button_width;
    layout->info_h = theme->button_height;

    layout->halt_x = layout->info_x + theme->button_width + theme->button_padding;
    layout->halt_y = layout->info_y;
    layout->halt_w = theme->button_width;
    layout->halt_h = theme->button_height;

    (void)fb;
}

void ui_draw(Framebuffer *fb, const UITheme *theme, const UIButtonLayout *layout, const char *time_text, const MouseState *mouse) {
    int time_x = (int)fb->width - ((int)8 * 8) - theme->button_padding;
    int time_y = (theme->bar_height - 8) / 2;

    fb_clear(fb, theme->bg_color);
    fb_draw_rect(fb, 0, 0, fb->width, theme->bar_height, theme->bar_color);

    {
        int hover = mouse && point_in_rect(mouse->x, mouse->y, layout->info_x, layout->info_y, layout->info_w, layout->info_h);
        fb_draw_rect(fb, layout->info_x, layout->info_y, layout->info_w, layout->info_h,
                     hover ? theme->button_hover : theme->button_color);
        fb_draw_text(fb, layout->info_x + 8, layout->info_y + 6, "Info", theme->text_color);
    }

    {
        int hover = mouse && point_in_rect(mouse->x, mouse->y, layout->halt_x, layout->halt_y, layout->halt_w, layout->halt_h);
        fb_draw_rect(fb, layout->halt_x, layout->halt_y, layout->halt_w, layout->halt_h,
                     hover ? theme->button_hover : theme->button_color);
        fb_draw_text(fb, layout->halt_x + 8, layout->halt_y + 6, "Halt", theme->text_color);
    }

    fb_draw_text(fb, time_x, time_y, time_text, theme->accent_color);

    if (mouse) {
        fb_draw_rect(fb, mouse->x, mouse->y, 6, 6, theme->accent_color);
    }
}

int ui_hit_test(const UIButtonLayout *layout, int x, int y) {
    if (point_in_rect(x, y, layout->info_x, layout->info_y, layout->info_w, layout->info_h)) {
        return 1;
    }
    if (point_in_rect(x, y, layout->halt_x, layout->halt_y, layout->halt_w, layout->halt_h)) {
        return 2;
    }
    return 0;
}
