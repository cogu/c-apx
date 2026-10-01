Server Extensions
=================

The APX server features a modular extension architecture that decouples network transports, transport-level security, logging, and runtime monitoring from the core signal routing engine.

.. toctree::
   :maxdepth: 2

   socket_server
   tls_server
   text_log
   monitor

Extension Architecture
----------------------

Extensions hook into the server lifecycle through standard registration handlers:

* **Static Registration**: Built-in extensions are registered during compilation using CMake (`apx_register_server_extension`) into a compile-time extension registry table.
* **Configuration Dispatch**: When `apx-server` loads its JSON configuration, `register_apx_server_extensions()` inspects top-level JSON keys formatted as ``"<name>-extension"`` and dispatches the corresponding configuration object to each extension.
* **Enabled Toggle**: Every extension supports an ``enabled`` boolean property. Setting ``"enabled": false`` prevents the extension from initializing at runtime.

.. code-block:: json

   {
     "socket-server-extension": {
       "enabled": true
     },
     "tls-server-extension": {
       "enabled": false
     },
     "textlog-extension": {
       "enabled": true
     },
     "monitor-extension": {
       "enabled": true
     }
   }

Compile-Time Exclusion & CMake Configuration
--------------------------------------------

In addition to disabling extensions dynamically via `server.json`, extensions can be excluded from the binary build entirely at compile time using CMake flags. When an extension is disabled at configure time, its target is omitted, its source code is not compiled, and its entry is excluded from the auto-generated static registry (`extensions_cfg.c`).

.. list-table::
   :header-rows: 1
   :widths: 40 15 45

   * - CMake Option
     - Default
     - Description
   * - ``APX_SERVER_ENABLE_SOCKET_EXTENSION``
     - ``ON``
     - Build and register the Socket Server Extension (UNIX domain socket, TCP/IP, systemd, VSOCK).
   * - ``APX_SERVER_ENABLE_TLS_EXTENSION``
     - ``ON``
     - Build and register the TLS Server Extension (mbedTLS / OpenSSL encrypted stream).
   * - ``APX_SERVER_ENABLE_TEXT_LOG_EXTENSION``
     - ``ON``
     - Build and register the Server Text Log Extension.
   * - ``APX_SERVER_ENABLE_MONITOR_EXTENSION``
     - ``ON``
     - Build and register the Server Monitor Extension.
   * - ``APX_SERVER_EXTENSION_DIRS``
     - ``""``
     - Semicolon-separated list of out-of-tree extension source directories.

Example: Minimal Server Build
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To compile a minimal server containing only the UNIX/TCP socket extension while omitting TLS, text logging, and runtime monitoring:

.. code-block:: bash

   cmake -B build \
     -DAPX_BUILD_SERVER=ON \
     -DAPX_SERVER_ENABLE_SOCKET_EXTENSION=ON \
     -DAPX_SERVER_ENABLE_TLS_EXTENSION=OFF \
     -DAPX_SERVER_ENABLE_TEXT_LOG_EXTENSION=OFF \
     -DAPX_SERVER_ENABLE_MONITOR_EXTENSION=OFF

Yocto / BitBake Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~

These options map directly to BitBake `PACKAGECONFIG` variables in custom meta layers.

For complete integration details, out-of-tree extension guides, and BitBake recipe examples, see the [APX CMake Integration Guide](https://github.com/cogu/c-apx/blob/master/cmake/README.md) (`cmake/README.md`).

Extension Catalog
-----------------

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Extension
     - Configuration Key
     - Capabilities
   * - :doc:`socket_server`
     - ``socket-server-extension``
     - Multi-transport listener supporting UNIX domain sockets, plain TCP/IP, systemd socket activation, Linux VSOCK (AF_VSOCK), and UNIX group credential whitelisting.
   * - :doc:`tls_server`
     - ``tls-server-extension``
     - High-security encrypted TLS listener with X.509 certificate validation, CA authority chains, and Mutual TLS (mTLS) client verification.
   * - :doc:`text_log`
     - ``textlog-extension``
     - Diagnostics and event text logging with configurable file output and standard output streams.
   * - :doc:`monitor`
     - ``monitor-extension``
     - Runtime monitoring extension for tracking connection states and active virtual files.
