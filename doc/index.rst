The Console library
===================

The libconsole is a simple abstraction of a basic command-line interface over a
serial (i.e. U(S)ART) device.

About the libconsole
--------------------

Principles
""""""""""

The goal of this library is **not** to implement an effective shell language or any advanced command-line mechanism, but
to define the necessary primitives to implement such a stack.

The libconsole provides an abstraction of the interaction between the U(S)ART interface and the task, by manipulating
lines and commands instead of .

Limitations
"""""""""""

The libconsole is not designed for holding multiple serial lines concurrently.



The libConsole API
------------------


Initializing the console
""""""""""""""""""""""""

Initialize the serial console is made through two main functions::

   #include "libconsole.h"

   mbed_error_t console_early_init(uint8_t         usart_id,
                                   uint32_t        speed);
   mbed_error_t console_init(void);


As usual, the early init function has to be called during the task
initialization phase. The init function has to be called at any moment of the
task nominal phase, before the begining of the console use.

.. warning::
   The libconsole does not support multiple multiple initializations (i.e. handling multiple serial lines in the same time)

The *usart_id* argument is the identifier of the USART device, as defined in
the bellow USART devices list (typically 1 to 6 on STM32F4xx SoCs). See the
used USART driver API to check which usart identifiers are supported by the
platform.

The *speed* argument is the serial console speed in bytes per second. Typical
speeds are one of the following:

   * 115200
   * 57600
   * 38400
   * 9200

It is possible to set a custom speed (such as 1Mb/s or older 1200 bps) but the
communication may not be supported by the peer.

Printing data on the console
""""""""""""""""""""""""""""

The console library provides a high level, easy to use logging function to
print-out formated strings on the serial console::

   #include "libconsole.h"

   void console_log(const char *fmt, ...);

This function supported formated output strings the way the printf() familly
functions of the EwoK libstd API handle them.

.. warning::
   This function handle up to 128 bytes length formated output string.
   Longer strings are truncated

Reading a line from the console
"""""""""""""""""""""""""""""""

When interacting with a peer through a serial console, a readline() function
is requested. This function is a blocking function returning the command
sent by the peer when a carriage return is sent.

Reading lines on the console interface is done with the following API::

   #include "libconsole.h"

   mbed_error_t console_readline(char *str, uint32_t *len, uint32_t maxlen);

This function is a blocking function, waiting for a command to be sent on the
serial line. The command is consider as sent (and the function is unlocking
its execution) when a *carriage return* is received.
When this event happen, the following is done:

   * The command sent is copied in the str argument, and may be truncated to maxlen if the command length is bigger than the current command length
   * The current command buffer is reinitialized
   * If the command copy truncate the received command, the function return
     MBED_ERROR_NOSTORAGE. Otherwise, the function return MBED_ERROR_NONE

.. hint::
   Any command parsing (shell level algorithmic) is under the task or another library responsability

Prompting
"""""""""

The libconsole support prompt printing on demand. The goal is to show a prompt
character, followed by a space, on the console.

This permit to print out shell-alike command prompt.

This is done by calling the following function::

   #include "libconsole.h"

   void console_show_prompt(void);

.. warning::
   This function is still in progress, as the prompt activation at init time is not yet implemented. By now, this function doesn't print-out anything

.. todo::
   Support prompt setting at init time


Libconsole FAQ
--------------

Is it possible to declare multiple serial console in a same task ?
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

Not by now. The libconsole is using a single global context which does not
permit to handle multiple serial lines in the same time.

Is it possible to read multiple lines at a time ?
"""""""""""""""""""""""""""""""""""""""""""""""""

Not at a time, but it is possible to read one line, parse it, and decide to
read another one if needed (for e.g. if the last non-space character is a
backslash)

Is the console USART device unmappable ?
""""""""""""""""""""""""""""""""""""""""

Not by now. the serial device is mapped automatically by the kernel.
