#include "io.h"
#include "mouse.h"

#define PS2_STATUS 0x64
#define PS2_DATA 0x60

#define PS2_STATUS_OUT 0x01
#define PS2_STATUS_IN 0x02
#define PS2_STATUS_AUX 0x20

static void ps2_wait_input(void) {
    while (inb(PS2_STATUS) & PS2_STATUS_IN) {
        io_wait();
    }
}

static void ps2_wait_output(void) {
    while (!(inb(PS2_STATUS) & PS2_STATUS_OUT)) {
        io_wait();
    }
}

static void ps2_write(uint8_t value) {
    ps2_wait_input();
    outb(PS2_DATA, value);
}

static void ps2_write_cmd(uint8_t value) {
    ps2_wait_input();
    outb(PS2_STATUS, value);
}

static uint8_t ps2_read(void) {
    ps2_wait_output();
    return inb(PS2_DATA);
}

static void mouse_write(uint8_t value) {
    ps2_write_cmd(0xD4);
    ps2_write(value);
    ps2_read();
}

void mouse_init(void) {
    ps2_write_cmd(0xA8);
    ps2_write_cmd(0x20);

    {
        uint8_t status = ps2_read();
        status |= 0x02;
        ps2_write_cmd(0x60);
        ps2_write(status);
    }

    mouse_write(0xF6);
    mouse_write(0xF4);
}

int mouse_poll(MouseState *state) {
    static uint8_t packet[3];
    static uint8_t index = 0;
    uint8_t status = inb(PS2_STATUS);

    if (!(status & PS2_STATUS_OUT)) {
        return 0;
    }
    if (!(status & PS2_STATUS_AUX)) {
        return 0;
    }

    packet[index++] = inb(PS2_DATA);
    if (index < 3) {
        return 0;
    }
    index = 0;

    if (!(packet[0] & 0x08)) {
        return 0;
    }

    {
        int dx = (int8_t)packet[1];
        int dy = (int8_t)packet[2];

        state->x += dx;
        state->y -= dy;
        state->buttons = packet[0] & 0x07;
    }

    return 1;
}
