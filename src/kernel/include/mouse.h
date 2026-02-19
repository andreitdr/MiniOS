#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
    int x;
    int y;
    uint8_t buttons;
} MouseState;

void mouse_init(void);
int mouse_poll(MouseState *state);

#endif
