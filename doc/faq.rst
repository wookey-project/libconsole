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
