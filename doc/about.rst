About the libconsole
--------------------

Principles
""""""""""

The libconsole is a simple abstraction of a basic command-line interface over a serial (i.e. U(S)ART) device.

The goal of this library is **not** to implement an effective shell language or any advanced command-line mechanism, but
to define the necessary primitives to implement such a stack.

The libconsole provides an abstraction of the interaction between the U(S)ART interface and the task, by manipulating
lines and commands instead of .

Limitations
"""""""""""

The libconsole is not designed for holding multiple serial lines concurrently.

