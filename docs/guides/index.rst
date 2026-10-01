Developer Guides
================

These guides explain how to build software using the ``c-apx`` C library, from embedding client runtimes and handling signal events to instrumenting servers and writing custom server extensions.

.. toctree::
   :maxdepth: 2

   c_client
   server_programming

Guides Overview
---------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Guide
     - Description
   * - :doc:`c_client`
     - How to write an APX client in C: client lifecycle, building nodes from APX IDL definitions, registering client event listeners (``connected``, ``disconnected``, ``require_port_write``), connecting over UNIX/TCP/TLS/VSOCK, and reading/writing port data.
   * - :doc:`server_programming`
     - How to program the APX server in C: server lifecycle, registering server and connection event listeners (``new_connection``, ``file_published``, logging), and authoring custom server extensions.
