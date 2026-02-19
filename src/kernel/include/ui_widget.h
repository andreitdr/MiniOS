#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <stdint.h>
#include "framebuffer.h"
#include "mouse.h"

typedef enum {
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_PANEL
} WidgetType;

typedef struct Widget Widget;

typedef void (*WidgetCallback)(Widget *widget, void *user_data);

struct Widget {
    WidgetType type;
    int x, y;
    int width, height;
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t hover_color;
    const char *text;
    int visible;
    int hovered;
    WidgetCallback on_click;
    void *user_data;
    Widget *next;
};

typedef struct {
    Widget *root;
    int widget_count;
} UIContext;

/* UI Context management */
void ui_context_init(UIContext *ctx);
void ui_context_free(UIContext *ctx);

/* Widget creation */
Widget* ui_create_button(const char *text, int x, int y, int width, int height);
Widget* ui_create_label(const char *text, int x, int y, uint32_t color);
Widget* ui_create_panel(int x, int y, int width, int height, uint32_t color);

/* Widget management */
void ui_add_widget(UIContext *ctx, Widget *widget);
void ui_set_callback(Widget *widget, WidgetCallback callback, void *user_data);
void ui_set_colors(Widget *widget, uint32_t bg, uint32_t fg, uint32_t hover);

/* Rendering */
void ui_render(UIContext *ctx, Framebuffer *fb);
void ui_render_widget(Widget *widget, Framebuffer *fb);

/* Event handling */
int ui_handle_mouse(UIContext *ctx, const MouseState *mouse, int clicked);

#endif
