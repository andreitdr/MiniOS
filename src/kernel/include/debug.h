#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

/* Log levels */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} LogLevel;

void debug_init(void);
void debug_set_level(LogLevel level);
void debug_putc(char c);
void debug_puts(const char *str);
void debug_puthex(uint32_t value);
void debug_log(const char *msg);
void debug_log_level(LogLevel level, const char *msg);

/* Convenience macros */
#define DEBUG(msg, ...) debug_log_level(LOG_DEBUG, msg)
#define INFO(msg, ...) debug_log_level(LOG_INFO, msg)
#define WARN(msg, ...) debug_log_level(LOG_WARN, msg)
#define ERROR(msg, ...) debug_log_level(LOG_ERROR, msg)

#endif
