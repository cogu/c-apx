apx-server
==========

The ``apx-server`` daemon is the central routing broker in an APX network. It accepts client node connections over multiple transport mechanisms, validates node definitions and cryptographic signatures, matches provide-ports with require-ports, and routes signal updates between nodes.

Synopsis
--------

.. code-block:: text

   apx-server [-h | --help] [--version]
              [-r <fd> | --ready-fd <fd>]
              [-s <json> | --socket-config <json>]
              [<config_file | config_dir>]

Command-Line Options
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-h``, ``--help``
     - Display command-line usage summary and exit.
   * - ``--version``
     - Display version information and exit.
   * - ``-r <fd>``, ``--ready-fd <fd>``
     - Readiness notification file descriptor. When provided, the server writes a single newline character (``\n``) to this file descriptor once all extensions are loaded and listener sockets are actively accepting connections, then closes the descriptor. This allows parent processes (such as test runners, systemd wrappers, and process supervisors) to synchronize deterministically without arbitrary sleep polling.
   * - ``-s <json>``, ``--socket-config <json>``
     - Inline JSON string providing configuration for the ``socket-server-extension``. When passed, it configures or overrides the socket server listener without modifying configuration files on disk. Must be a valid JSON object.
   * - ``<config_file | config_dir>``
     - Optional path to a JSON configuration file or a directory containing a configuration file.

Configuration Resolution
------------------------

When launched with a positional path argument:

1. **Directory Path**: If the argument points to a directory, ``apx-server`` searches for ``server.json``. If ``server.json`` is not found, it falls back to ``apx_server.json``.
2. **File Path**: If the argument points directly to a file, the file is loaded and parsed as JSON.
3. **CLI Socket Override**: If ``-s`` / ``--socket-config`` is provided alongside a file or directory configuration, the parsed inline socket configuration replaces any ``"socket-server-extension"`` entry from the configuration file.
4. **No Config Argument**: If no path is provided and ``--socket-config`` is supplied, a blank server configuration is created and populated exclusively with the socket settings.

Server Core Configuration Schema
--------------------------------

Server configuration files are formatted as JSON objects. The core server options are specified under the ``"apx-server"`` key:

.. code-block:: json

   {
     "apx-server": {
       "security": {
         "require-signed-nodes": true,
         "trusted-keys": [
           "/etc/apx/certs/trusted_node_pubkey.pem"
         ]
       },
       "max-num-events": 200,
       "apx-cache-enabled": false,
       "apx-cache-path": ""
     }
   }

Configuration Parameters
~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - Key
     - Type
     - Description
   * - ``security.require-signed-nodes``
     - Boolean
     - When set to ``true``, client nodes connecting to the server must supply a valid cryptographic signature accompanying their APX definition (RMFP/1.1). Unsigned nodes or nodes signed with keys not in ``trusted-keys`` are rejected. Defaults to ``false``. Also accepts key name ``require_signed_nodes``.
   * - ``security.trusted-keys``
     - Array | String
     - List of file paths to trusted public key PEM files (or inline PEM strings). Signatures presented by client nodes are verified against these keys. Also accepts key name ``trusted_keys``.
   * - ``max-num-events``
     - Integer
     - Capacity of internal event queue for routing operations.
   * - ``apx-cache-enabled``
     - Boolean
     - *(Placeholder / Not yet implemented)* Reserved for enabling/disabling node parse tree caching.
   * - ``apx-cache-path``
     - String
     - *(Placeholder / Not yet implemented)* Reserved for filesystem directory path used for node caching.

Extension Configuration Sections
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Additional top-level keys in the configuration file configure registered server extensions:

* ``"socket-server-extension"``: Configures UNIX domain socket, TCP/IP, and Linux VSOCK endpoints, systemd socket activation, and UNIX group whitelisting. See :doc:`../extensions/socket_server`.
* ``"tls-server-extension"``: Configures encrypted TLS listeners and Mutual TLS (mTLS) client verification. See :doc:`../extensions/tls_server`.
* ``"textlog-extension"``: Configures server event text logging to stdout or file. See :doc:`../extensions/text_log`.
* ``"monitor-extension"``: Configures runtime server monitoring. See :doc:`../extensions/monitor`.

Daemon Lifecycle & Signal Handling
----------------------------------

Signals
~~~~~~~

* **``SIGTERM`` / ``SIGINT``**: Initiates graceful shutdown. The server terminates worker threads, detaches connected clients, stops all server extensions, unlinks UNIX domain socket files, and frees all allocated memory.
* **``SIGPIPE``**: Automatically ignored by the server to ensure client disconnects or write failures on closed sockets never terminate the daemon.

Readiness Notification (``--ready-fd``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When spawning ``apx-server`` as a child process or under service managers, use ``--ready-fd``:

.. code-block:: python

   # Example: Deterministic Python / pytest startup
   r_fd, w_fd = os.pipe()
   os.set_inheritable(w_fd, True)

   proc = subprocess.Popen(
       ["apx-server", "--ready-fd", str(w_fd), "-s", json.dumps({"unix-file": "/tmp/apx.socket"})],
       pass_fds=(w_fd,)
   )
   os.close(w_fd)

   # Block until server signals readiness byte
   ready = os.read(r_fd, 1)
   os.close(r_fd)

Systemd Integration
-------------------

``apx-server`` supports socket activation under systemd. In this mode, systemd binds `/run/apx/apx.socket` and passes the pre-bound file descriptor (descriptor 3, ``SD_LISTEN_FDS_START``) upon launch:

.. code-block:: ini

   # /etc/systemd/system/apx-server.socket
   [Unit]
   Description=APX Server Socket Activation
   PartOf=apx-server.service

   [Socket]
   ListenStream=/run/apx/apx.socket
   SocketMode=0660
   SocketUser=apx
   SocketGroup=apx-users

   [Install]
   WantedBy=sockets.target

.. code-block:: ini

   # /etc/systemd/system/apx-server.service
   [Unit]
   Description=APX Server Daemon
   Requires=apx-server.socket
   After=network.target apx-server.socket

   [Service]
   Type=simple
   ExecStart=/usr/bin/apx-server /etc/apx/server.json
   Restart=on-failure
   User=apx
   Group=apx

To enable socket activation in ``server.json``:

.. code-block:: json

   {
     "socket-server-extension": {
       "enabled": true,
       "unix-systemd": true,
       "unix-tag": "local"
     }
   }

Usage Examples
--------------

1. Launch with Configuration File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-server /etc/apx/server.json

2. Quick Ad-Hoc UNIX Domain Socket
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-server --socket-config '{"unix-file": "/tmp/apx.socket"}'

3. Dual TCP and UNIX Domain Socket with Group Whitelisting
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-server --socket-config '{
     "unix-file": "/tmp/apx.socket",
     "allowed-groups": ["apx-users", "wheel"],
     "tcp-port": 5000
   }'
