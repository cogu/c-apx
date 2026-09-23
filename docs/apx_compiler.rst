APX Bytecode Compiler (apx_compiler)
====================================

The ``apx_compiler`` compiles APX port definitions and signatures into bytecode programs (``apx_program_t``) for execution by the APX virtual machine.

Overview
--------

The compiler translates high-level port data signatures (e.g. integer types, arrays, records) into linear bytecode programs.

API Reference
-------------

Data Types
~~~~~~~~~~

.. doxygentypedef:: apx_compiler_t

Lifecycle Functions
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_compiler_create
.. doxygenfunction:: apx_compiler_destroy
.. doxygenfunction:: apx_compiler_new
.. doxygenfunction:: apx_compiler_delete

Compilation Functions
~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_compiler_compile_port
