c-apx
=====

**c-apx** is the C implementation of `APX <https://apx.readthedocs.io>`_.

Overview
--------

**c-apx** is the reference implementation of the APX standards and specifications:

* **Reference Implementation**: Implements the normative APX specifications,
  defining how APX nodes, files, and signal routing behave.
* **Server Implementation**: It is currently the only APX implementation that
  implements the APX server (``apx-server``), matching provide and require
  ports and routing signal updates across connected nodes. Other language
  implementations (such as Python and C++) only implement the client-side
  of APX.
* **Specification Support**: Implements the **APX IDL v1.3** and **APX VM v2.1**
  specifications, as well as RMFP/1.1 signed file transfers.

Features
--------

* **Complete APX Stack**: Implements both the client runtime (``apx-client``)
  and server daemon (``apx-server``), alongside interactive CLI tools
  (``apx-control``, ``apx-node``, ``apx-sign``, ``apx_perf_test``).
* **Standards Conformance**: Full compliance with APX IDL v1.3 and
  APX VM v2.1 execution models.
* **Server Routing & Extensibility**: Dynamic port matching and signal
  routing with an extensible architecture supporting transport listeners
  (UNIX domain sockets, TCP/IP, Linux VSOCK), TLS / Mutual TLS (mTLS),
  text logging, and runtime monitoring.
* **Security & Authentication**: Cryptographic node definition verification
  using ECDSA NIST P-256 (secp256r1) with SHA-256 (RMFP/1.1) and UNIX peer
  credential access control lists.
* **Strict C99 Design**: High-performance, portable C99 codebase with strict
  memory ownership patterns and a lightweight footprint suitable for desktop
  applications and embedded environments.

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Getting Started

   getting_started

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Developer Guides

   guides/index

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Applications

   apps/index

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Server Extensions

   extensions/index

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: C API Reference

   api/index

Getting Started
===============

Follow the :doc:`getting_started` guide for a hands-on walkthrough building ``c-apx``, starting the server daemon, connecting sample sender/listener nodes, and transmitting live signal values using ``apx-control``.

Developer Guides
================

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Guide
     - Description
   * - :doc:`guides/c_client`
     - Writing APX clients in C: node definitions, client lifecycle, client event listeners, and signal read/write.
   * - :doc:`guides/server_programming`
     - Server programming in C: embedding ``apx_server_t``, server and connection event listeners, and custom extensions.

Applications
============

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - Application
     - Binary
     - Description
   * - :doc:`apps/apx_server`
     - ``apx-server``
     - Central daemon routing signals, enforcing security, and managing extensions.
   * - :doc:`apps/apx_node`
     - ``apx-node``
     - Interactive node runtime with JSON router socket and signature verification.
   * - :doc:`apps/apx_control`
     - ``apx-control``
     - Command-line utility to get/set signal values on running nodes.
   * - :doc:`apps/apx_info`
     - ``apx-info``
     - Query tool for inspecting server status and connected clients.
   * - :doc:`apps/apx_sign`
     - ``apx-sign``
     - Cryptographic signing tool (ECDSA NIST P-256) for APX definition files.
   * - :doc:`apps/apx_perf_test`
     - ``apx_perf_test``
     - High-speed round-trip ping-pong throughput benchmark.

Server Extensions
=================

.. list-table::
   :header-rows: 1
   :widths: 25 30 45

   * - Extension
     - Config Key
     - Description
   * - :doc:`extensions/socket_server`
     - ``socket-server-extension``
     - UNIX domain sockets, TCP/IP, systemd socket activation, Linux VSOCK, and group whitelists.
   * - :doc:`extensions/tls_server`
     - ``tls-server-extension``
     - Encrypted TLS stream transport and Mutual TLS (mTLS) client verification.
   * - :doc:`extensions/text_log`
     - ``textlog-extension``
     - Text event logging to console (stdout) or log files.
   * - :doc:`extensions/monitor`
     - ``monitor-extension``
     - Runtime server monitoring and connection tracking.

C API Modules
=============

.. list-table::
   :header-rows: 1
   :widths: 20 25 55

   * - Module
     - Header
     - Description
   * - :doc:`api/apx_server`
     - ``apx/server.h``
     - Central APX server daemon managing client connections, signal routing, and extensions.
   * - :doc:`api/apx_client`
     - ``apx/client.h``
     - Client runtime managing node registration, server connection, and signal exchange.
   * - :doc:`api/apx_node`
     - ``apx/node.h``
     - Parse tree representation of an APX node, port definitions, and signal instances.
   * - :doc:`api/apx_file_manager`
     - ``apx/file_manager.h``
     - Remote File Protocol manager handling memory-mapped virtual file transfers.
   * - :doc:`api/apx_vm`
     - ``apx/vm.h``
     - Virtual machine executing bytecode programs to serialize/deserialize port data.
   * - :doc:`api/apx_compiler`
     - ``apx/compiler.h``
     - In-memory compiler generating bytecode programs from APX port signatures.
   * - :doc:`api/apx_types`
     - ``apx/types.h``
     - Core constants, port directions, handle identifiers, and shared types.
   * - :doc:`api/apx_error`
     - ``apx/error.h``
     - Standardized error codes and diagnostic error string translation.
