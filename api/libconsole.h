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
#ifndef LIBCONSOLE_H_
#define LIBCONSOLE_H_

/*
 * This is a basic serial console implementation, which permit interactions with
 * a user through a generic prompt.
 *
 * Though, this library is *not* a shell as it does not implement any shell-relative
 * language. Although, it is perfectly possible to use the libconsole primitives
 * to implement a complete serial shell like, for e.g. ash.
 *
 * Info: the libconsole support backspace feature and user command echoing.
 *
 */

#include "api/types.h"

/**
 * \brief log a message to the console
 *
 * The fmt format support all the length modifiers and flags that are
 * supported by the printf() libstd familly (see api/stdio.h).
 *
 * \param fmt the output message format
 * \param ... the variable arguments list
 */
void console_log(const char *fmt, ...);

/*
 * \brief Show the console prompt
 *
 * INFO: This is still a future usage as the prompt char initialization need
 * to be set at console_init() call. The goal is to allow the usage of prompt
 * marks like '$ ' or '> ' by specifying the prompt character at init time.
 *
 * By now, the console_show_prompt() function just do nothing as the prompt
 * flag is locked.
 *
 */
void console_show_prompt(void);

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
mbed_error_t console_readline(char *str, uint32_t *len, uint32_t maxlen);


/*
 * Initialize U(S)ART in console mode.
 * This function declare the serial console to the kernel.
 */
mbed_error_t console_early_init(uint8_t         usart_id,
                                uint32_t        speed);

/*
 * Initialize the serial device itself
 *
 * This function initialize the device, and require the device to be mapped.
 * This function should be called after the initialization phase of the task.
 */
mbed_error_t console_init(void);

#endif
