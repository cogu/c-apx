Applications
============

``c-apx`` includes a suite of command-line applications and utilities for running servers, simulating nodes, inspecting signal traffic, benchmarking performance, and managing cryptographic signatures.

.. toctree::
   :maxdepth: 2

   apx_server
   apx_node
   apx_control
   apx_info
   apx_sign
   apx_perf_test

Overview of Applications
------------------------

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - Application
     - Binary
     - Purpose
   * - :doc:`apx_server`
     - ``apx-server``
     - Central daemon routing signals between provide and require ports, managing client connections, enforcing cryptographic node security, and coordinating server extensions.
   * - :doc:`apx_node`
     - ``apx-node``
     - Interactive node runtime. Connects to the server over UNIX domain sockets, TCP, TLS, or VSOCK; parses APX definitions; verifies companion signatures; exposes a JSON router interface to read/write signals.
   * - :doc:`apx_control`
     - ``apx-control``
     - Command-line utility for interacting with an ``apx-node``'s JSON router interface to get/set signal values or inject signal batches from JSON files.
   * - :doc:`apx_info`
     - ``apx-info``
     - Query client that inspects connected nodes and active files on an APX server.
   * - :doc:`apx_sign`
     - ``apx-sign``
     - Cryptographic signing tool implementing RMFP/1.1 file signing and verification using ECDSA NIST P-256 (secp256r1) and SHA-256 with PEM key management.
   * - :doc:`apx_perf_test`
     - ``apx_perf_test``
     - High-speed round-trip ping-pong benchmark measuring signal throughput and latency across the APX server daemon.
