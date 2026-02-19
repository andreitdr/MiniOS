#include <stdint.h>
#include <stddef.h>

#include "bootinfo.h"
#include "framebuffer.h"
#include "mouse.h"
#include "time.h"
#include "ui_widget.h"
#include "debug.h"

/* Global UI state */
static Framebuffer g_fb;
static int show_info = 0;

static void clamp_mouse(MouseState *mouse, const Framebuffer *fb) {
    if (mouse->x < 0) {
        mouse->x = 0;
    }
    if (mouse->y < 0) {
        mouse->y = 0;
    }
    if (mouse->x >= fb->width) {
        mouse->x = fb->width - 1;
    }
    if (mouse->y >= fb->height) {
        mouse->y = fb->height - 1;
    }
}

/* Button callbacks */
static void on_info_clicked(Widget *widget, void *user_data) {
    (void)widget;
    (void)user_data;
    debug_log("Info button clicked");
    debug_puts("show_info was: ");
    debug_puthex(show_info);
    debug_puts("\r\n");
    show_info = !show_info;
    debug_puts("show_info now: ");
    debug_puthex(show_info);
    debug_puts("\r\n");
}

static void on_halt_clicked(Widget *widget, void *user_data) {
    (void)widget;
    (void)user_data;
    __asm__ volatile ("cli; hlt");
}

void kmain(struct BootInfo *info) {
    UIContext ui_ctx;
    MouseState mouse = { 40, 40, 0 };
    char time_text[9] = "12:00:00";
    Widget *top_bar, *btn_info, *btn_halt, *lbl_time;
    Widget *info_panel, *info_bg, *lbl_info_title, *lbl_res, *lbl_res_val, *lbl_bpp, *lbl_bpp_val;

    debug_init();
    debug_log("Kernel started");

    if (info->magic != BOOTINFO_MAGIC || info->bpp != 32) {
        debug_log("Boot info invalid!");
        for (;;) {
            __asm__ volatile ("hlt");
        }
    }

    debug_log("Initializing framebuffer");
    fb_init(&g_fb, info);
    debug_log("Initializing mouse");
    mouse_init();
    mouse.x = g_fb.width / 2;
    mouse.y = g_fb.height / 2;

    debug_log("Initializing UI");
    /* Initialize UI */
    ui_context_init(&ui_ctx);

    /* Create top bar */
    top_bar = ui_create_panel(0, 0, g_fb.width, 28, 0x3B4A68);
    ui_add_widget(&ui_ctx, top_bar);

    /* Create buttons */
    btn_info = ui_create_button("Info", 8, 4, 96, 20);
    ui_set_callback(btn_info, on_info_clicked, NULL);
    ui_add_widget(&ui_ctx, btn_info);

    btn_halt = ui_create_button("Halt", 112, 4, 96, 20);
    ui_set_callback(btn_halt, on_halt_clicked, NULL);
    ui_add_widget(&ui_ctx, btn_halt);

    /* Create time label */
    lbl_time = ui_create_label(time_text, g_fb.width - 72, 10, 0xB4D5FF);
    ui_add_widget(&ui_ctx, lbl_time);

    /* Create info panel widgets (hidden initially) */
    int panel_w = 320;
    int panel_h = 120;
    int panel_x = (g_fb.width - panel_w) / 2;
    int panel_y = (g_fb.height - panel_h) / 2;

    info_panel = ui_create_panel(panel_x, panel_y, panel_w, panel_h, 0x2A2F3A);
    info_panel->visible = 0;
    ui_add_widget(&ui_ctx, info_panel);

    info_bg = ui_create_panel(panel_x + 2, panel_y + 2, panel_w - 4, panel_h - 4, 0x1B1E24);
    info_bg->visible = 0;
    ui_add_widget(&ui_ctx, info_bg);

    lbl_info_title = ui_create_label("Display info", panel_x + 12, panel_y + 16, 0xF1F4F8);
    lbl_info_title->visible = 0;
    ui_add_widget(&ui_ctx, lbl_info_title);

    lbl_res = ui_create_label("Resolution:", panel_x + 12, panel_y + 32, 0xF1F4F8);
    lbl_res->visible = 0;
    ui_add_widget(&ui_ctx, lbl_res);

    lbl_res_val = ui_create_label("800x600", panel_x + 120, panel_y + 32, 0xB4D5FF);
    lbl_res_val->visible = 0;
    ui_add_widget(&ui_ctx, lbl_res_val);

    lbl_bpp = ui_create_label("BPP:", panel_x + 12, panel_y + 48, 0xF1F4F8);
    lbl_bpp->visible = 0;
    ui_add_widget(&ui_ctx, lbl_bpp);

    lbl_bpp_val = ui_create_label("32", panel_x + 120, panel_y + 48, 0xB4D5FF);
    lbl_bpp_val->visible = 0;
    ui_add_widget(&ui_ctx, lbl_bpp_val);

    /* Main loop */
    for (;;) {
        uint8_t prev_buttons = mouse.buttons;
        int needs_redraw = 0;
        int clicked = 0;

        if (mouse_poll(&mouse)) {
            clamp_mouse(&mouse, &g_fb);
            needs_redraw = 1;
        }

        if ((mouse.buttons & 0x01) && !(prev_buttons & 0x01)) {
            clicked = 1;
        }

        /* Handle mouse events */
        if (ui_handle_mouse(&ui_ctx, &mouse, clicked)) {
            debug_log("Mouse event handled");
            /* Update info panel visibility */
            if (info_panel) info_panel->visible = show_info;
            if (info_bg) info_bg->visible = show_info;
            if (lbl_info_title) lbl_info_title->visible = show_info;
            if (lbl_res) lbl_res->visible = show_info;
            if (lbl_res_val) lbl_res_val->visible = show_info;
            if (lbl_bpp) lbl_bpp->visible = show_info;
            if (lbl_bpp_val) lbl_bpp_val->visible = show_info;
            debug_log("Panel visibility updated");
            needs_redraw = 1;
        }

        if (needs_redraw) {
            debug_log("Redrawing");
            /* Clear background */
            fb_clear(&g_fb, 0x1C2433);
            
            debug_log("Rendering widgets");
            /* Render all widgets */
            ui_render(&ui_ctx, &g_fb);
            
            debug_log("Drawing cursor");
            /* Draw mouse cursor */
            fb_draw_rect(&g_fb, mouse.x, mouse.y, 6, 6, 0xB4D5FF);
            debug_log("Redraw complete");
        }

        for (volatile int delay = 0; delay < 10000; delay++);
    }
}
