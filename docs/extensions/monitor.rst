Server Monitor Extension
========================

The ``monitor-extension`` provides dedicated diagnostic, introspection, and live streaming capabilities within the APX server daemon. It powers the :doc:`../apps/apx_mon` CLI tool by exposing an interactive diagnostic endpoint and event notification stream over RemoteFile v1.1 stream files (``FileStream``).

Definition inspection returns validated logical APX text, never an incomplete
or unverified compressed transfer buffer. Live and snapshot ``server.log``
definition events use the same uncompressed bytes and logical sizes.
``NODE_DEFINITION_REJECTED`` provides encoding and failure metadata without
requiring logging-side decompression; see :doc:`../apps/apx_log`.

Configuration Key
-----------------

In ``server.json``, the extension is configured under the ``"monitor-extension"`` key:

.. code-block:: json

   {
     "monitor-extension": {
       "enabled": true,
       "socket-file": "/tmp/apx.socket",
       "tcp-port": 5080,
       "bind-address": "127.0.0.1"
     }
   }

Configuration Parameters
------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 15 15 45

   * - Parameter
     - Type
     - Default
     - Description
   * - ``enabled``
     - Boolean
     - ``true``
     - Enables or disables the server monitoring extension.
   * - ``socket-file``
     - String
     - *None*
     - Path to the UNIX domain socket for diagnostic connections (can also be specified as ``unix-file``).
   * - ``tcp-port``
     - Integer
     - *None*
     - TCP port number for diagnostic connections (e.g. ``5080``; can also be specified as ``port``).
   * - ``bind-address``
     - String
     - *None*
     - Local network interface address to bind the TCP listener to (e.g. ``"127.0.0.1"``).
   * - ``tls``
     - Object
     - *None*
     - Optional TLS configuration for securing diagnostic traffic (see below).

TLS Configuration Parameters
----------------------------

When the ``tls`` object is configured, diagnostic connections require TLS encryption:

.. list-table::
   :header-rows: 1
   :widths: 25 15 15 45

   * - Parameter
     - Type
     - Default
     - Description
   * - ``server-cert``
     - String
     - *None*
     - Path to the server certificate file (PEM format).
   * - ``server-key``
     - String
     - *None*
     - Path to the server private key file (PEM format).
   * - ``ca-cert``
     - String
     - *None*
     - Path to the trusted CA certificate file (PEM format).
   * - ``require-client-cert``
     - Boolean
     - ``true``
     - Require mutual TLS (mTLS) authentication from connecting diagnostic clients.

Usage
-----

The monitor extension observes server lifecycle hooks without blocking signal routing performance. When idle, zero state tracking overhead is incurred. Diagnostic clients such as :doc:`../apps/apx_mon` can query active connections, list registered nodes and routes, fetch node definitions, and subscribe to real-time signal change notifications.
