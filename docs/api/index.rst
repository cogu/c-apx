API Reference
=============

The APX C library provides a modular API for embedding APX communication, virtual file management, bytecode execution, and server routing into C applications.

.. toctree::
   :maxdepth: 2

   apx_server
   apx_client
   apx_node
   apx_file_manager
   apx_vm
   apx_compiler
   apx_types
   apx_error

Module Overview
---------------

.. list-table::
   :header-rows: 1
   :widths: 20 25 55

   * - Module
     - Header
     - Description
   * - :doc:`apx_server`
     - ``apx/server.h``
     - Server daemon managing client connections, signal routing, and extensions.
   * - :doc:`apx_client`
     - ``apx/client.h``
     - Client runtime managing node registration, server connection, and signal exchange.
   * - :doc:`apx_node`
     - ``apx/node.h``
     - Parse tree representation of an APX node, port definitions, and signal instances.
   * - :doc:`apx_file_manager`
     - ``apx/file_manager.h``
     - Remote File Protocol manager handling memory-mapped virtual file transfers.
   * - :doc:`apx_vm`
     - ``apx/vm.h``
     - Virtual machine executing bytecode programs to serialize/deserialize port data.
   * - :doc:`apx_compiler`
     - ``apx/compiler.h``
     - In-memory compiler generating bytecode programs from APX port signatures.
   * - :doc:`apx_types`
     - ``apx/types.h``
     - Core constants, port directions, handle identifiers, and shared types.
   * - :doc:`apx_error`
     - ``apx/error.h``
     - Standardized error codes and diagnostic error string translation.
