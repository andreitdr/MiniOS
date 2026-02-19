#include <stdint.h>

static inline void bios_putc(char c) {
    __asm__ volatile (
        "movb $0x0E, %%ah\n"
        "movb %0, %%al\n"
        "xor %%bh, %%bh\n"
        "int $0x10\n"
        :
        : "r" (c)
        : "ax", "bx"
    );
}

static void puts(const char *s) {
    while (*s) {
        bios_putc(*s++);
    }
}

void kmain(void) {
    puts("Hello world from C kernel!\r\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
