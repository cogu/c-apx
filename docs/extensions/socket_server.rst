Socket Server Extension
=======================

The ``socket-server-extension`` provides standard streaming network transports for the APX server daemon. It supports UNIX domain sockets, plain TCP/IP, Linux VSOCK (AF_VSOCK) for hypervisor/guest communication, systemd socket activation, and peer credential access control.

Configuration Key
-----------------

In ``server.json``, the extension is configured under the ``"socket-server-extension"`` key:

.. code-block:: json

   {
     "socket-server-extension": {
       "enabled": true,
       "unix-file": "/tmp/apx.socket",
       "tcp-port": 5000
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
     - Toggles extension registration and execution.
   * - ``unix-file``
     - String
     - *None*
     - Filesystem path for creating a UNIX domain socket listener (e.g. ``/tmp/apx.socket`` or ``/run/apx/apx.socket``). Automatically removed and unlinked on graceful daemon shutdown.
   * - ``unix-tag``
     - String
     - ``""``
     - Diagnostic tag label applied to connections received on the UNIX domain socket.
   * - ``unix-systemd``
     - String | Boolean
     - ``false``
     - Configures systemd socket activation mode. Supports ``"auto"``, ``true`` / ``1``, and ``false`` / ``0``. Can also be specified as ``"systemd"``. See below for details.
   * - ``allowed-groups``
     - Array | String
     - *None*
     - Access control list of UNIX group names permitted to connect. Peer credentials (UID/GID) are inspected on connection. If the client does not belong to any allowed group, the server closes the connection immediately.
   * - ``tcp-port``
     - Integer
     - *None*
     - Port number on which to bind a plain TCP listener (valid range: ``1024`` to ``65535``). Listens on all interfaces (``0.0.0.0``).
   * - ``tcp-tag``
     - String
     - ``""``
     - Diagnostic tag label applied to connections received on the TCP socket.
   * - ``vsock-port``
     - Integer
     - *None*
     - Linux VSOCK (AF_VSOCK) port number. When specified, starts a VSOCK listener for communication between host and guest virtual machines without IP networking.
   * - ``vsock-cid``
     - Integer
     - ``0xFFFFFFFF`` (ANY)
     - Linux VSOCK Context ID to bind to. Default is ``VMADDR_CID_ANY`` (accept connections from any CID).
   * - ``vsock-tag``
     - String
     - ``""``
     - Diagnostic tag label applied to connections received on the VSOCK endpoint.

Features & Transports
---------------------

1. UNIX Domain Sockets & Access Control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

UNIX domain sockets offer the highest performance and lowest latency on Linux systems:

.. code-block:: json

   {
     "socket-server-extension": {
       "unix-file": "/run/apx/apx.socket",
       "unix-tag": "local",
       "allowed-groups": ["apx-users", "wheel"]
     }
   }

When a client connects to the UNIX socket:

1. The server audits the client process using kernel peer credentials (``SO_PEERCRED``): retrieving PID, UID, and GID.
2. If ``allowed-groups`` is configured, the server verifies that the client's primary GID or supplementary group list matches at least one configured group.
3. If unauthorized, an access denied warning is logged and the connection is closed immediately without exchanging data.

2. Systemd Socket Activation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``unix-systemd`` parameter controls adoption of inherited pre-bound socket descriptors:

* **``"auto"``**: Queries inherited descriptors via ``sd_listen_fds()``. If file descriptor 3 (``SD_LISTEN_FDS_START``) is present, the server adopts it. If no file descriptors are present, it falls back to creating the socket file specified in ``unix-file``.
* **``true`` / ``1``**: Strictly requires socket activation. If no file descriptor was passed by systemd, the server logs an error and fails initialization.
* **``false`` / ``0``**: Disables socket activation and creates the socket using ``unix-file``.

.. code-block:: json

   {
     "socket-server-extension": {
       "unix-systemd": "auto",
       "unix-file": "/run/apx/apx.socket",
       "unix-tag": "systemd"
     }
   }

3. Linux VSOCK (AF_VSOCK) Transport
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

VSOCK provides fast, isolated bidirectional streaming between host processes and virtual machines (or across hypervisor boundaries) without configuring virtual ethernet bridges:

.. code-block:: json

   {
     "socket-server-extension": {
       "vsock-port": 52000,
       "vsock-cid": 4294967295,
       "vsock-tag": "vsock"
     }
   }

4. Dual UNIX & TCP Listener
~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can configure both UNIX domain and TCP listeners concurrently:

.. code-block:: json

   {
     "socket-server-extension": {
       "unix-file": "/tmp/apx.socket",
       "unix-tag": "local",
       "tcp-port": 5000,
       "tcp-tag": "lan"
     }
   }
