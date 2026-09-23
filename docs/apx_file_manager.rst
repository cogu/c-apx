Remote File Protocol Manager (apx_file_manager)
===============================================

The ``apx_file_manager`` coordinates memory-mapped virtual files exchanged between nodes and servers using the Remote File Protocol (RMFP).

Overview
--------

In APX, all signal data and definition information are exposed as virtual memory-mapped files:

* ``NodeName.apx``: APX text specification describing node types and ports.
* ``NodeName.out``: Byte buffer containing provide port (output) signal values.
* ``NodeName.in``: Byte buffer containing require port (input) signal values.

The file manager tracks file states (open, requested, synced), generates address mappings, and handles low-level write/read messages.

API Reference
-------------

Data Types
~~~~~~~~~~

.. doxygentypedef:: apx_file_manager_t

Lifecycle Functions
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_file_manager_create
.. doxygenfunction:: apx_file_manager_destroy
.. doxygenfunction:: apx_file_manager_start
.. doxygenfunction:: apx_file_manager_stop

File Operations
~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_file_manager_create_local_file
.. doxygenfunction:: apx_file_manager_publish_local_file
.. doxygenfunction:: apx_file_manager_find_file_by_address
.. doxygenfunction:: apx_file_manager_find_local_file_by_name
.. doxygenfunction:: apx_file_manager_find_remote_file_by_name
.. doxygenfunction:: apx_file_manager_send_local_const_data
