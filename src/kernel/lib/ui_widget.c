#include "ui_widget.h"
#include "framebuffer.h"
#include "debug.h"
#include <stddef.h>

/* Simple memory allocator for widgets - fixed pool */
#define MAX_WIDGETS 32
static Widget widget_pool[MAX_WIDGETS];
static int widget_pool_used = 0;

static Widget* alloc_widget(void) {
    if (widget_pool_used >= MAX_WIDGETS) {
        return NULL;
    }
    Widget *w = &widget_pool[widget_pool_used++];
    w->type = WIDGET_BUTTON;
    w->x = 0;
    w->y = 0;
    w->width = 0;
    w->height = 0;
    w->bg_color = 0x4F5F7A;
    w->fg_color = 0xF1F4F8;
    w->hover_color = 0x60739A;
    w->text = NULL;
    w->visible = 1;
    w->hovered = 0;
    w->on_click = NULL;
    w->user_data = NULL;
    w->next = NULL;
    return w;
}

void ui_context_init(UIContext *ctx) {
    ctx->root = NULL;
    ctx->widget_count = 0;
    widget_pool_used = 0;
}

void ui_context_free(UIContext *ctx) {
    ctx->root = NULL;
    ctx->widget_count = 0;
    widget_pool_used = 0;
}

Widget* ui_create_button(const char *text, int x, int y, int width, int height) {
    Widget *w = alloc_widget();
    if (!w) return NULL;
    
    w->type = WIDGET_BUTTON;
    w->text = text;
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->bg_color = 0x4F5F7A;
    w->fg_color = 0xF1F4F8;
    w->hover_color = 0x60739A;
    w->visible = 1;
    
    return w;
}

Widget* ui_create_label(const char *text, int x, int y, uint32_t color) {
    Widget *w = alloc_widget();
    if (!w) return NULL;
    
    w->type = WIDGET_LABEL;
    w->text = text;
    w->x = x;
    w->y = y;
    w->width = 0;  /* Auto-size based on text */
    w->height = 8;
    w->fg_color = color;
    w->visible = 1;
    
    return w;
}

Widget* ui_create_panel(int x, int y, int width, int height, uint32_t color) {
    Widget *w = alloc_widget();
    if (!w) return NULL;
    
    w->type = WIDGET_PANEL;
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->bg_color = color;
    w->visible = 1;
    
    return w;
}

void ui_add_widget(UIContext *ctx, Widget *widget) {
    if (!widget) return;
    
    widget->next = ctx->root;
    ctx->root = widget;
    ctx->widget_count++;
}

void ui_set_callback(Widget *widget, WidgetCallback callback, void *user_data) {
    if (!widget) return;
    widget->on_click = callback;
    widget->user_data = user_data;
}

void ui_set_colors(Widget *widget, uint32_t bg, uint32_t fg, uint32_t hover) {
    if (!widget) return;
    widget->bg_color = bg;
    widget->fg_color = fg;
    widget->hover_color = hover;
}

static int point_in_widget(const Widget *widget, int x, int y) {
    return (x >= widget->x && x < widget->x + widget->width &&
            y >= widget->y && y < widget->y + widget->height);
}

void ui_render_widget(Widget *widget, Framebuffer *fb) {
    if (!widget || !widget->visible || !fb) return;
    
    switch (widget->type) {
        case WIDGET_BUTTON: {
            uint32_t color = widget->hovered ? widget->hover_color : widget->bg_color;
            fb_draw_rect(fb, widget->x, widget->y, widget->width, widget->height, color);
            
            if (widget->text) {
                int text_x = widget->x + 8;
                int text_y = widget->y + ((widget->height - 8) / 2);
                fb_draw_text(fb, text_x, text_y, widget->text, widget->fg_color);
            }
            break;
        }
        
        case WIDGET_LABEL:
            if (widget->text) {
                fb_draw_text(fb, widget->x, widget->y, widget->text, widget->fg_color);
            }
            break;
        
        case WIDGET_PANEL:
            fb_draw_rect(fb, widget->x, widget->y, widget->width, widget->height, widget->bg_color);
            break;
    }
}

void ui_render(UIContext *ctx, Framebuffer *fb) {
    Widget *w;
    
    if (!ctx || !fb || !ctx->root) {
        return;
    }
    
    /* Simple forward rendering - render widgets from oldest to newest */
    /* We need to reverse the list first since root points to newest */
    Widget *stack[MAX_WIDGETS];
    int count = 0;
    
    /* Build array of widgets */
    w = ctx->root;
    while (w && count < MAX_WIDGETS) {
        stack[count++] = w;
        w = w->next;
    }
    
    /* Render from bottom (oldest) to top (newest) */
    for (int i = count - 1; i >= 0; i--) {
        if (stack[i]) {
            ui_render_widget(stack[i], fb);
        }
    }
}

int ui_handle_mouse(UIContext *ctx, const MouseState *mouse, int clicked) {
    Widget *w;
    int handled = 0;
    
    if (!ctx || !mouse) {
        return 0;
    }
    
    w = ctx->root;
    
    /* Update hover states and handle clicks */
    while (w) {
        if (w->visible && w->type == WIDGET_BUTTON) {
            w->hovered = point_in_widget(w, mouse->x, mouse->y);
            
            if (clicked && w->hovered) {
                if (w->on_click) {
                    /* Adjust function pointer: kernel loaded at 0x20000 but linked at 0x0 */
                    WidgetCallback actual_callback = w->on_click;
                    if ((uint32_t)actual_callback < 0x20000) {
                        actual_callback = (WidgetCallback)((uint32_t)actual_callback + 0x20000);
                    }
                    
                    actual_callback(w, w->user_data);
                    handled = 1;
                    break; /* Only handle one click at a time */
                }
            }
        }
        w = w->next;
    }
    
    return handled;
}
