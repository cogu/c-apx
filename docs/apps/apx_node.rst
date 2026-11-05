apx-node
========

``apx-node`` is an interactive APX client node runner. It instantiates an APX node based on an input definition
file (``.apx``), connects to the ``apx-server`` over a variety of network transports, publishes provide-ports, observes
require-ports, and exposes a JSON message router socket for external tools to inspect and alter signal values at
runtime.

Synopsis
--------

.. code-block:: text

   apx-node [-b | --bind <bind_path>] [-p | --bind-port <port>] [--no-bind]
            [-c | --connect <connect_path>] [-r | --connect-port <port>]
            [--no-signature] [--compressed]
            [--tls] [--ca-cert <path>] [--client-cert <path>] [--client-key <path>]
            [--vsock <cid>:<port>]
            [--version] [--help]
            <file.apx>

Command-Line Options
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``<file.apx>``
     - Path to the APX definition file describing the node, port interfaces, and data types (mandatory argument).
   * - ``-b <path>``, ``--bind <path>``
     - Path to UNIX domain socket or TCP IP address on which ``apx-node`` starts its local JSON message server.
       External utilities (such as :doc:`apx_control` or ``nc``) connect here to manipulate provide-ports.
       Default on Linux: ``/tmp/apx_node.socket``. Default on Windows: ``127.0.0.1``.
   * - ``-p <port>``, ``--bind-port <port>``
     - Port number for the local JSON message server when binding over TCP. Default: ``5100``.
   * - ``--no-bind``
     - Disable the local JSON message server. In this mode, ``apx-node`` runs purely as an observer and signal
       publisher.
   * - ``-c <path>``, ``--connect <path>``
     - Path to UNIX domain socket or hostname/IP address of the remote ``apx-server``. Default on Linux:
       ``/tmp/apx.socket``. Default on Windows: ``127.0.0.1``.
   * - ``-r <port>``, ``--connect-port <port>``
     - Port number to connect to on the ``apx-server``. Defaults to ``5000`` for plain TCP, or ``5020`` when ``--tls``
       is specified.
   * - ``--tls``
     - Connect to ``apx-server`` over encrypted TLS. Automatically defaults port to ``5020``.
   * - ``--ca-cert <path>``
     - Path to Root CA certificate PEM file to verify server authenticity when connecting via TLS.
   * - ``--client-cert <path>``
     - Path to client certificate PEM file for Mutual TLS (mTLS) authentication.
   * - ``--client-key <path>``
     - Path to client private key PEM file for Mutual TLS (mTLS) authentication.
   * - ``--no-signature``
     - Bypass automatic discovery of companion signature files and publish the node unsigned.
   * - ``--compressed``
     - Transfer the APX definition using zlib-wrapped DEFLATE at the backend's default level
       (currently 6). Takes no argument. Without this option, definitions are sent as plain text.
       Works with signed or unsigned definitions and requires a compression-capable server.
       Signal data and initial values remain uncompressed; there is no automatic fallback.
   * - ``--vsock <cid>:<port>``
     - Connect to ``apx-server`` using Linux VSOCK (AF_VSOCK) transport. The ``<cid>`` parameter can be a numeric
       Context ID or alias: ``host`` (2), ``local`` (1), ``hypervisor`` (0), or ``any`` (0xFFFFFFFF).
   * - ``--version``
     - Display version information and exit.
   * - ``-h``, ``--help``
     - Display usage help and exit.

Core Capabilities
-----------------

1. JSON Signal Router Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, ``apx-node`` opens a local control socket (UNIX socket at ``/tmp/apx_node.socket`` or TCP). External tools
(such as :doc:`apx_control`, shell utilities like ``nc`` or ``socat``, or test scripts) connect to this socket and
send Newline-Delimited JSON (NDJSON) messages to set provide-port values:

.. code-block:: json

   {"VehicleSpeed": 72.5}

When ``apx-node`` receives this command, it serializes the value into the node's local output virtual file
(``<Node>.out``) using the APX VM bytecode, which triggers transmission to the server.

2. Require-Port Monitoring
~~~~~~~~~~~~~~~~~~~~~~~~~~

When any require-port on the node receives updated data from the APX server, ``apx-node`` deserializes the payload and
prints the signal change directly to standard output as formatted JSON:

.. code-block:: text

   [APX-SIGNAL] VehicleSpeed: 72.5

3. Cryptographic Signature Discovery (RMFP/1.1)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When supplied with ``EngineNode.apx``, ``apx-node`` checks for companion signature files in the same directory:

* ``EngineNode.apx.sig``
* ``EngineNode.sig``

If found, ``apx-node`` reads the 64-byte raw binary ECDSA NIST P-256 signature, initializes the Remote File Protocol
in version 1.1 mode, and authenticates the definition file to the server.

Usage Examples
--------------

Compressed Definition Transfer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select definition compression explicitly:

.. code-block:: bash

   apx-node --compressed -c /tmp/apx.socket EngineNode.apx

Companion signatures are discovered as usual and authenticate the original
definition text. Add ``--no-signature`` to publish unsigned. Compression is
honored even when the encoded definition is larger than its plain-text form.
There is currently no command-line compression-level option; applications
using the C client API can select a level explicitly.

Standard Local Run
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-node nodes/SensorNode.apx

Connect with Custom Socket Paths
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-node -c /tmp/apx_custom.socket -b /tmp/sensor_ctrl.socket nodes/SensorNode.apx

Connect Over TLS with Mutual Authentication (mTLS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-node --tls \
     --ca-cert certs/ca_cert.pem \
     --client-cert certs/client_cert.pem \
     --client-key certs/client_key.pem \
     -c 127.0.0.1 -r 5020 \
     nodes/SecureNode.apx

Connect Over Linux VSOCK (Guest to Host)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-node --vsock 2:52000 nodes/SensorNode.apx

Send Signal Updates via Netcat (NDJSON)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   echo '{"VehicleSpeed": 75}' | nc -U /tmp/apx_node.socket
