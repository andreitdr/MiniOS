#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <stdint.h>

#define BOOTINFO_MAGIC 0x544F4F42u

struct BootInfo {
    uint32_t magic;
    uint32_t lfb;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint8_t bpp;
    uint8_t reserved;
    uint32_t font_ptr;
    uint8_t boot_drive;
    uint8_t pad[3];
} __attribute__((packed));

#endif
