/*
 *
 * Copyright 2019 The wookey project team <wookey@ssi.gouv.fr>
 *   - Ryad     Benadjila
 *   - Arnauld  Michelizza
 *   - Mathieu  Renard
 *   - Philippe Thierry
 *   - Philippe Trebuchet
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * ur option) any later version.
 *
 * This package is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this package; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */
#include "libc/types.h"
#include "libc/syscall.h"
#include "libc/stdio.h"
#include "libc/stdarg.h"
#include "libc/string.h"
#include "libc/nostd.h"
#include "libusart.h"
#include "libconsole.h"

#define MAX_CONSOLE_CMD_SIZE 128

/*
 * getc and putc handlers, which will be fullfill by the usart driver through
 * the config structure, at init time
 */
static cb_usart_getc_t console_getc = NULL;
static cb_usart_putc_t console_putc = NULL;


/*
 * libconsole global context
 */
typedef struct {
    char             prompt_char;
    bool             has_prompt;
    volatile bool    command_received;
    bool             console_initialized;
    uint8_t          uart_id;
    uint8_t          command_length;
    char             command[MAX_CONSOLE_CMD_SIZE];
} console_context_t;

console_context_t console_ctx = {
    .prompt_char = '\0',
    .has_prompt = false,
    .command_received = false,
    .console_initialized = false,
    .uart_id = 0,
    .command_length = 0,
    .command = { 0 }
};

/*
 * libconsole global serial console configuration
 */
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
    .callback_usart_getc_ptr = &console_getc,
    .callback_usart_putc_ptr = &console_putc
};

/**********************************************************
 * Various libconsole utilities and handlers
 *********************************************************/

/*
 * console fmt support and printout
 */
static void console_print(const char *fmt, va_list args)
{
    uint32_t i = 0;
    char string[128];

    memset(string, 0x0, 128*sizeof(char));
    vsnprintf(string, 127, fmt, args);
    while (string[i] != '\0' && i < 128) {
       console_putc(string[i++]);
    }
}

/*
 * console input handler (when receiving chars on the line)
 */
static void cb_console_irq_handler(uint32_t sr, uint32_t dr)
{
    if (sr & 0x20) {
      // copy the char in the console ring buffer
      // print char in console too
      if ((dr & 0xff) != '\r') {
          /* backspace support */
          if ((dr & 0xff) == '\b') {
              if (console_ctx.command_length > 0) {
                  /* echoing backspace */
                  console_putc(dr & 0xff);
                  console_putc(0x20);
                  console_putc(dr & 0xff);
                  /* removing last char */
                  console_ctx.command_length--;
                  console_ctx.command[console_ctx.command_length] = '\0';
              }
          } else {
              /* handling each char. Too big commands are just trunkated in
               * the buffer, but still printed out on the console */
              if (console_ctx.command_length < MAX_CONSOLE_CMD_SIZE) {
                  console_ctx.command[console_ctx.command_length++] = dr & 0xff;
              }
              /* echoing command */
              console_putc(dr & 0xff);
         }
      } else {
          /* crriage return received. echoing new line + carriage return */
          console_putc('\r');
          console_putc('\n');
          console_ctx.command_received = 1;
      }
    }
}

/******************************************************
 * libconsole exported API
 *****************************************************/

/**
 * \brief log a message to the console
 *
 * The fmt format support all the length modifiers and flags that are
 * supported by the printf() libstd familly (see libc/stdio.h).
 *
 * \param fmt the output message format
 * \param ... the variable arguments list
 */
void console_log(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    console_print(fmt, args);
    va_end(args);
}

/*
 * \brief Show the console prompt
 *
 * INFO: This is still a future usage as the prompt char initialization need
 * to be set at console_init() call. The goal is to allow the usage of prompt
 *  marks like '$ ' or '> ' by specifying the prompt character at init time.
 *
 * By now, the console_show_prompt() function just do nothing as the prompt
 * flag is locked.
 *
 */
void console_show_prompt(void)
{
    if (console_ctx.has_prompt) {
        console_log("%c ", console_ctx.prompt_char);
    }
}

/*
 * \brief read a line from the serial console
 *
 * Wait until the user return a command line, finishing with a carriage return.
 * This is a blocking function, returning the command typed by the user (without
 * the end of line marker).
 * The command may be trunkated if:
 * 1) the given maxlen is smaller than the command size
 * 2) the command passed on the serial line is bigger than the libconsole command
 *    buffer (i.e. 128 bytes)
 *
 *  \param str the output buffer
 *  \param len the command length that have been read
 *  \param maxlen the max command length the output buffer can hold
 *
 *  \return MBED_ERROR_NONE if everything is ok
 *          MBED_ERROR_NOSTORAGE if the command had to be trunkated due to a small
 *            output buffer
 *
 */
mbed_error_t console_readline(char *str, uint32_t *len, uint32_t maxlen)
{
    mbed_error_t ret = MBED_ERROR_NONE;

    if (str == NULL || len == NULL) {
        ret = MBED_ERROR_INVPARAM;
        goto err;
    }
    /* By now, the readline action is a blocking action, meaning that the
     * task can't handle external non-ISR events such as IPC while it waits
     * for a command.
     * TODO: a non-blocking implementation would be useful for more complex
     * tasks */
    while (1) {
        if (console_ctx.command_received) {
            /* If needed, the received command is trunkated to the user
             * buffer size */
            uint32_t to_copy = console_ctx.command_length <= maxlen ? console_ctx.command_length : maxlen;
            strncpy(str, console_ctx.command, to_copy);
            *len = to_copy;

            /* if there is not enough space in the user provided buffer, the
             * function return NOSTORAGE as informational */
            if (console_ctx.command_length > to_copy) {
                ret = MBED_ERROR_NOSTORAGE;
            }

            /* clearing the command buffer */
            memset(console_ctx.command, 0x0, sizeof(console_ctx.command));
            console_ctx.command_length = 0;
            console_ctx.command_received = false;
            return ret;
        }
        sys_yield();
    }
err:
    return ret;
}

/*
 * Initialize the serial device itself
 *
 * This function initialize the device, and require the device to be mapped.
 * This function should be called after the initialization phase of the task.
 */
mbed_error_t console_init(void)
{
    mbed_error_t ret = MBED_ERROR_NONE;

    if (usart_init(&config)) {
        ret = MBED_ERROR_INITFAIL;
        goto err;
    }
    console_ctx.uart_id = config.usart;
    console_ctx.console_initialized = true;
err:
    return ret;
}

/*
 * Initialize U(S)ART in console mode.
 * This function declare the serial console to the kernel.
 */
mbed_error_t console_early_init(uint8_t         usart_id,
                                uint32_t        speed)
{
    mbed_error_t ret = MBED_ERROR_NONE;

    config.set_mask = USART_SET_ALL;
    config.mode = UART;
    config.usart = usart_id;
    config.baudrate = speed;
    config.word_length = USART_CR1_M_8;
    config.stop_bits = USART_CR2_STOP_1BIT;
    config.parity = USART_CR1_PCE_DIS;
    config.hw_flow_control = USART_CR3_CTSE_CTS_DIS | USART_CR3_RTSE_RTS_DIS;
    config.options_cr1 = USART_CR1_TE_EN | USART_CR1_RE_EN | USART_CR1_UE_EN | USART_CR1_RXNEIE_EN;
    config.callback_irq_handler = cb_console_irq_handler;
    /* INFO: getc and putc handler in config are set by the USART driver at early init */

    if (usart_early_init(&config, USART_MAP_AUTO)) {
        ret = MBED_ERROR_INITFAIL;
    }

    return ret;
}

