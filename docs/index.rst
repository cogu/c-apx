c-apx
=====

**c-apx** is the C implementation of `APX <https://apx.readthedocs.io>`_.

Overview
--------

**c-apx** is the reference implementation of the APX standards and specifications:

* **Reference Implementation**: Implements the normative APX specifications,
  defining how APX nodes, files, and signal routing behave.
* **Server Implementation**: It is currently the only APX implementation that
  implements the APX server (``apx_server``), matching provide and require
  ports and routing signal updates across connected nodes. Other language
  implementations (such as Python and C++) only implement the client-side
  of APX.
* **Specification Support**: Implements the **APX IDL v1.3** and **APX VM v2.1**
  specifications.

Features
--------

* **Complete APX Stack**: Implements both the client runtime (``apx_client``)
  and server daemon (``apx_server``), alongside interactive CLI tools
  (``apx_control``, ``apx_node``).
* **Standards Conformance**: Full compliance with APX IDL v1.3 and
  APX VM v2.1 execution models.
* **Server Routing & Extensibility**: Dynamic port matching and signal
  routing with an extensible architecture supporting transport listeners
  (TCP/IP and UNIX domain sockets), text logging, and runtime monitoring.
* **Strict C99 Design**: High-performance, portable C99 codebase with strict
  memory ownership patterns and a lightweight footprint suitable for desktop
  applications and embedded environments.

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: API Reference

   apx_server
   apx_client
   apx_node
   apx_file_manager
   apx_vm
   apx_compiler
   apx_types
   apx_error

Components Catalog
==================

Below is a summary of core modules provided by the c-apx library:

.. list-table::
   :header-rows: 1
   :widths: 20 25 55

   * - Module
     - Header
     - Description
   * - :doc:`apx_server`
     - ``apx/server.h``
     - Central APX server daemon managing client connections, signal routing, and extensions.
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
