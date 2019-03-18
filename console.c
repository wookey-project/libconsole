#include "api/types.h"
#include "api/syscall.h"
#include "api/stdio.h"
#include "api/nostd.h"
#include "api/libusart.h"
#include "api/libconsole.h"

#define BUF_SIZE	512
#define BUF_MAX		(BUF_SIZE - 1)
#define PUT_CHAR(c)					\
	console_ring_buffer.buf[console_ring_buffer.end++] = c;		\
	console_ring_buffer.end %= BUF_MAX;			\
	if (console_ring_buffer.end == console_ring_buffer.start) {	\
		console_ring_buffer.start++;			\
		console_ring_buffer.start %= BUF_MAX;		\
	}

cb_usart_getc_t console_getc = NULL;
cb_usart_putc_t console_putc = NULL;

void console_flush(void);
void console_log(const char *fmt, ...);

static void console_print(const char *fmt, va_list args);

static struct {
    uint32_t start;
    uint32_t end;
    char buf[BUF_SIZE];
} console_ring_buffer;

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

void panic(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    console_print(fmt, args);
    va_end(args);
    console_flush();
    while (1)
        continue;
}

void init_console_ring_buffer(void)
{
    int i = 0;
    console_ring_buffer.end = 0;
    console_ring_buffer.start = console_ring_buffer.end;

    for (i = 0; i < BUF_MAX; i++) {
        console_ring_buffer.buf[i] = '\0';
    }
}

static void console_write_digit(uint8_t digit)
{
    if (digit < 0xa)
        digit += '0';
    else
        digit += 'a' - 0xa;
    PUT_CHAR(digit);
}

static void console_copy_string(char *str, uint32_t len)
{
    uint32_t size =
        len < (BUF_MAX - console_ring_buffer.end) ? len : BUF_MAX - console_ring_buffer.end;
    strncpy(console_ring_buffer.buf + console_ring_buffer.end, str, size);
    uint32_t dist = console_ring_buffer.start - console_ring_buffer.end;
    if (console_ring_buffer.end < console_ring_buffer.start && dist < size) {
        console_ring_buffer.start += size - dist + 1;
        console_ring_buffer.start %= BUF_MAX;
    }
    console_ring_buffer.end += size;
    console_ring_buffer.end %= BUF_MAX;
    if (len - size)
        console_copy_string(str + size, len - size);
}


void console_flush(void)
{
    if (console_putc == NULL) {
        panic("Error: console_putc not initialized");
    }
    while (console_ring_buffer.start != console_ring_buffer.end) {
        console_putc(console_ring_buffer.buf[console_ring_buffer.start++]);
        console_ring_buffer.start %= BUF_MAX;
    }
}


static void console_print(const char *fmt, va_list args)
{
    uint32_t i = 0;
    char *string;

    for (i = 0; fmt[i]; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
            case 'd':
                itoa(va_arg(args, uint32_t), 10);
                break;
            case 'x':
                PUT_CHAR('0');
                PUT_CHAR('x');
                itoa(va_arg(args, uint32_t), 16);
                break;
            case '%':
                PUT_CHAR('%');
                break;
            case 's':
                string = va_arg(args, char *);
                console_copy_string(string, strlen(string));
                break;
            case 'l':
                if (fmt[i + 1] == 'l' && fmt[i + 2] == 'd') {
                    itoa(va_arg(args, unsigned long long), 10);
                    i += 2;
                } else if (fmt[i + 1] == 'd') {
                    itoa(va_arg(args, unsigned long), 10);
                    i++;
                }
                break;
            case 'c':
                PUT_CHAR((unsigned char)va_arg(args, int));
                break;
            default:
                PUT_CHAR('?');
            }
        } else if (fmt[i] == '\n' && fmt[i + 1] != '\r') {
            console_copy_string("\n\r", 2);
        } else {
            PUT_CHAR(fmt[i]);
        }
    }
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
        console_putc(dr & 0xff);
      } else {
        // new line + carriage return
        console_putc('\r');
        console_putc('\n');
      }
  } else {
    // nothing...
//    printf("%c", dr & 0xff);
  }
}

uint8_t console_init(void)
{
    return (usart_init(&config));
}

uint8_t console_early_init(uint8_t usart_id, uint32_t speed, console_handler handler)
{
    init_console_ring_buffer();

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
    config.callback_usart_getc_ptr = &console_getc;
    config.callback_usart_putc_ptr = &console_putc;

    return (usart_early_init(&config, USART_MAP_AUTO));
}

