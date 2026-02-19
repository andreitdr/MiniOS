#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>

void bios_putc(char c);
uint8_t bios_get_key(void);
int bios_get_drive_params(uint8_t drive, uint16_t *cylinders, uint8_t *heads, uint8_t *spt);

#endif
