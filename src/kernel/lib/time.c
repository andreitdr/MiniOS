#include "time.h"

struct BiosTimeBcd {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

extern void bios_get_time_bcd_raw(void);
extern struct BiosTimeBcd bios_time_bcd;

static uint8_t bcd_to_bin(uint8_t value) {
    return (uint8_t)(((value >> 4) * 10) + (value & 0x0F));
}

void time_get_hms(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    bios_get_time_bcd_raw();
    *hours = bcd_to_bin(bios_time_bcd.hours);
    *minutes = bcd_to_bin(bios_time_bcd.minutes);
    *seconds = bcd_to_bin(bios_time_bcd.seconds);
}
