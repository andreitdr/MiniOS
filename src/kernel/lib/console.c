#include "bios.h"
#include "console.h"

void console_puts(const char *s) {
    while (*s) {
        bios_putc(*s++);
    }
}

void console_put_crlf(void) {
    bios_putc('\r');
    bios_putc('\n');
}

void console_put_uint(uint32_t value) {
    char buf[11];
    int i = 0;

    if (value == 0) {
        bios_putc('0');
        return;
    }

    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        bios_putc(buf[--i]);
    }
}
