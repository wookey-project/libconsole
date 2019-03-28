#include "api/types.h"
#include "api/syscall.h"
#include "api/stdio.h"
#include "api/stdarg.h"
#include "api/string.h"
#include "api/nostd.h"
#include "api/libusart.h"
#include "api/libconsole.h"

usart_config_t config = {
    .set_mask = 0,
    .usart = 0,
    .mode = UART,
    .baudrate = 0,
    .word_length = 0,
    .parity = 0,
    .stop_bits = 0,
    .hw_flow_control = 0,
    .options_cr1 = 0,
    .options_cr2 = 0,
    .guard_time_prescaler = 0,
    .callback_irq_handler = NULL,
    .callback_usart_getc_ptr = NULL,
    .callback_usart_putc_ptr = NULL
};

static void console_print(const char *fmt, va_list args)
{
    uint32_t i = 0;
    char string[128];

    memset(string, 0x0, 128*sizeof(char));
    vsnprintf(string, 127, fmt, args);
    while (string[i] != '\0' && i < 128) {
       ((cb_usart_putc_t)config.callback_usart_putc_ptr)(string[i++]);
    }
}

void panic(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    console_print(fmt, args);
    va_end(args);
    while (1)
        continue;
}

void console_log(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    console_print(fmt, args);
    va_end(args);
}

void cb_console_irq_handler(uint32_t sr __attribute__((unused)), uint32_t dr)
{
  // Receiving?
  // RXNE : bit 5 (starting with 0)
  if (sr & 0x20) {
      if ((dr & 0xff) != '\r') {
          ((cb_usart_putc_t)config.callback_usart_putc_ptr)(dr & 0xff);
      } else {
          // new line + carriage return
          ((cb_usart_putc_t)config.callback_usart_putc_ptr)('\r');
          ((cb_usart_putc_t)config.callback_usart_putc_ptr)('\n');
      }
  }
}

uint8_t console_init(void)
{
    return (usart_init(&config));
}

uint8_t console_early_init(uint8_t usart_id, uint32_t speed, console_handler handler)
{
    config.set_mask = USART_SET_ALL;
    config.mode = UART;
    config.usart = usart_id;
    config.baudrate = speed;
    config.word_length = USART_CR1_M_8;
    config.stop_bits = USART_CR2_STOP_1BIT;
    config.parity = USART_CR1_PCE_DIS;
    config.hw_flow_control = USART_CR3_CTSE_CTS_DIS | USART_CR3_RTSE_RTS_DIS;
    config.options_cr1 = USART_CR1_TE_EN | USART_CR1_RE_EN | USART_CR1_UE_EN | USART_CR1_RXNEIE_EN;
    if (handler) {
        config.callback_irq_handler = handler;
    } else {
        config.callback_irq_handler = cb_console_irq_handler;
    }
    /* INFO: getc and putc handler in config are set by the USART driver at early init */

    usart_early_init(&config, USART_MAP_AUTO);

    return 0;
}

