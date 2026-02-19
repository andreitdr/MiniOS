#include "bios.h"

void bios_putc(char c) {
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

uint8_t bios_get_key(void) {
    uint8_t al;

    __asm__ volatile (
        "xor %%ah, %%ah\n"
        "int $0x16\n"
        "movb %%al, %0\n"
        : "=r" (al)
        :
        : "ax"
    );

    return al;
}

int bios_get_drive_params(uint8_t drive, uint16_t *cylinders, uint8_t *heads, uint8_t *spt) {
    uint16_t cx = 0;
    uint16_t dx = drive;
    uint8_t carry;

    __asm__ volatile (
        "int $0x13\n"
        "setc %0\n"
        : "=q" (carry), "=c" (cx), "+d" (dx)
        : "a" (0x0800)
        : "bx"
    );

    if (carry) {
        return 0;
    }

    {
        uint8_t ch = (uint8_t)(cx >> 8);
        uint8_t cl = (uint8_t)(cx & 0xFF);
        uint8_t dh = (uint8_t)(dx >> 8);

        *spt = (uint8_t)(cl & 0x3F);
        *heads = (uint8_t)(dh + 1);
        *cylinders = (uint16_t)((((uint16_t)(cl & 0xC0)) << 2) | ch);
    }
    *cylinders = (uint16_t)(*cylinders + 1);

    return 1;
}
