#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

void console_puts(const char *s);
void console_put_crlf(void);
void console_put_uint(uint32_t value);

#endif
