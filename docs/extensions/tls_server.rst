TLS Server Extension
====================

The ``tls-server-extension`` adds encrypted transport capabilities to the APX server daemon using Transport Layer Security (TLS). It supports both standard server-side TLS authentication and Mutual TLS (mTLS) client certificate verification.

Configuration Key
-----------------

In ``server.json``, the extension is configured under the ``"tls-server-extension"`` key:

.. code-block:: json

   {
     "tls-server-extension": {
       "enabled": true,
       "tcp-port": 5020,
       "server-cert": "/etc/apx/certs/server_cert.pem",
       "server-key": "/etc/apx/certs/server_key.pem",
       "ca-cert": "/etc/apx/certs/ca_cert.pem",
       "require-client-cert": false,
       "tag": "TLS"
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
   * - ``tcp-port``
     - Integer
     - *None*
     - TCP port number to bind the TLS listener on (valid range: ``1024`` to ``65535``). Common default is ``5020``. Also accepts key name ``port``.
   * - ``server-cert``
     - String
     - *None*
     - Filesystem path to the server's X.509 certificate PEM file. Also accepts key name ``certificate``. Mandatory when enabling TLS.
   * - ``server-key``
     - String
     - *None*
     - Filesystem path to the server's private key PEM file. Also accepts key name ``key``. Mandatory when enabling TLS.
   * - ``ca-cert``
     - String
     - *None*
     - Filesystem path to the trusted Root CA certificate PEM file. Used to authenticate client certificates in mTLS mode.
   * - ``require-client-cert``
     - Boolean
     - ``false``
     - When set to ``true``, enables Mutual TLS (mTLS). Connecting clients must present a valid certificate signed by the Certificate Authority in ``ca-cert``; otherwise, the TLS handshake is rejected.
   * - ``tag``
     - String
     - ``"TLS"``
     - Diagnostic tag label applied to connections received on this listener. Also accepts key name ``tcp-tag``.

Operating Modes
---------------

1. Server Authentication (One-Way TLS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In this mode, client nodes verify the server identity, and all port data exchanged over the wire is encrypted. Clients do not need certificates.

.. code-block:: json

   {
     "tls-server-extension": {
       "tcp-port": 5020,
       "server-cert": "certs/server_cert.pem",
       "server-key": "certs/server_key.pem",
       "require-client-cert": false
     }
   }

2. Mutual TLS (mTLS) Authentication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In high-security automotive and edge deployments, configure ``require-client-cert: true``. This ensures only authorized clients with valid certificates issued by the designated Certificate Authority can establish connections with the APX server:

.. code-block:: json

   {
     "tls-server-extension": {
       "tcp-port": 5020,
       "server-cert": "certs/server_cert.pem",
       "server-key": "certs/server_key.pem",
       "ca-cert": "certs/ca_cert.pem",
       "require-client-cert": true,
       "tag": "mTLS"
     }
   }

Connecting Clients Over TLS
---------------------------

When connecting using :doc:`../apps/apx_node`, specify the ``--tls`` flag:

.. code-block:: bash

   # Connect with server authentication:
   apx-node --tls --ca-cert certs/ca_cert.pem -c 127.0.0.1 -r 5020 node.apx

   # Connect with mutual TLS (mTLS):
   apx-node --tls \
     --ca-cert certs/ca_cert.pem \
     --client-cert certs/client_cert.pem \
     --client-key certs/client_key.pem \
     -c 127.0.0.1 -r 5020 node.apx
