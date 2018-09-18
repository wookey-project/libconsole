#ifndef LIBCONSOLE_H_
#define LIBCONSOLE_H_

#include "api/types.h"

typedef void (*console_handler)(uint32_t sr, uint32_t dr);

void console_log(const char *fmt, ...);

void console_flush(void);

uint8_t console_init(void);

/*
 * Initialize U(S)ART in console mode. If handler is null, libconsole will
 * use its own, which will only print back any received char.
 */
uint8_t console_early_init(uint8_t usart_id, uint32_t speed, console_handler handler);

#endif
