#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

void debug_init(void);
void debug_putc(char c);
void debug_puts(const char *str);
void debug_puthex(uint32_t value);
void debug_log(const char *msg);

#endif
