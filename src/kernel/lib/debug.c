#include "debug.h"

#define COM1_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void debug_init(void) {
    /* Initialize COM1: 9600 baud, 8N1 */
    outb(COM1_PORT + 1, 0x00);    /* Disable interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB */
    outb(COM1_PORT + 0, 0x0C);    /* Divisor low byte (9600 baud) */
    outb(COM1_PORT + 1, 0x00);    /* Divisor high byte */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO */
    outb(COM1_PORT + 4, 0x0B);    /* Enable interrupts, RTS/DSR set */
    
    debug_puts("Debug initialized\r\n");
}

static int is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void debug_putc(char c) {
    while (!is_transmit_empty());
    outb(COM1_PORT, (uint8_t)c);
}

void debug_puts(const char *str) {
    if (!str) return;
    
    /* Use simple pointer access without adjustment for debugging */
    const char *s = str;
    
    /* Try to detect if we need pointer adjustment */
    uintptr_t ptr = (uintptr_t)str;
    if (ptr < 0x10000) {
        s = (const char *)(ptr + 0x20000);
    }
    
    for (int i = 0; i < 256 && s[i] != '\0'; i++) {
        debug_putc(s[i]);
    }
}

void debug_puthex(uint32_t value) {
    const char hex[] = "0123456789ABCDEF";
    debug_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        debug_putc(hex[(value >> i) & 0xF]);
    }
}

void debug_log(const char *msg) {
    debug_puts("[DEBUG] ");
    debug_puts(msg);
    debug_puts("\r\n");
}
