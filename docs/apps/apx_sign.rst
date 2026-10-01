apx-sign
========

``apx-sign`` is a cryptographic signing and verification utility for APX node definition files (``.apx``). It implements the Remote File Protocol version 1.1 (RMFP/1.1) specification for tamper-proof virtual file distribution and authenticating nodes to the APX server daemon.

Synopsis
--------

.. code-block:: text

   apx-sign [command] [options] <file.apx>

Algorithms & Formats
--------------------

* **Algorithm**: ECDSA over the NIST P-256 (secp256r1) curve with SHA-256 message digests.
* **Deterministic Signatures (RFC 6979)**: Powered by Mbed TLS, producing reproducible signatures without external random number generator requirements during signing.
* **Signature Format**: Raw 64-byte binary format consisting of concatenated 32-byte big-endian integers $(r, s)$.
* **Key Format**: Standard PEM-encoded files (``EC PRIVATE KEY`` and ``PUBLIC KEY`` SubjectPublicKeyInfo).

Commands
--------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Command
     - Description
   * - ``sign``
     - Sign an APX definition file using an ECDSA private key PEM (default action). Generates a companion ``.sig`` file.
   * - ``verify``
     - Verify an APX definition file against an ECDSA public key PEM using its companion signature.
   * - ``keygen [prefix]``
     - Generate a new ECDSA NIST P-256 keypair in PEM format. Defaults to prefix ``node`` (creating ``node_key.pem`` and ``node_pubkey.pem``).

Command-Line Options
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-k <file>``, ``--key <file>``
     - Path to the private key PEM file (in ``sign`` mode) or public key PEM file (in ``verify`` mode).
   * - ``-o <file>``, ``--output <file>``
     - Custom output filename for the generated binary signature file or key prefix for ``keygen``.
   * - ``-s <file>``, ``--sig <file>``
     - Explicit path to the signature file to verify. When omitted, automatically resolves ``<file.apx>.sig`` or ``<file>.sig``.
   * - ``-v``, ``--verify``
     - Switch operation mode to verification.
   * - ``-g``, ``--keygen``
     - Switch operation mode to key generation.
   * - ``-h``, ``--help``
     - Display usage help and exit.
   * - ``-V``, ``--version``
     - Display version information and exit.

Usage Examples
--------------

1. Generate a New Keypair
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Generates node_key.pem and node_pubkey.pem
   apx-sign keygen

   # Generates certs/ecu_key.pem and certs/ecu_pubkey.pem
   apx-sign keygen certs/ecu

2. Sign an APX Definition File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Signs SensorNode.apx and outputs companion file SensorNode.apx.sig
   apx-sign -k certs/ecu_key.pem SensorNode.apx

3. Verify a Signature Manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Verifies SensorNode.apx using SensorNode.apx.sig and certs/ecu_pubkey.pem
   apx-sign verify -k certs/ecu_pubkey.pem SensorNode.apx

4. Enforcing Signatures in Server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add the generated public key to your ``server.json`` to enforce verification:

.. code-block:: json

   {
     "apx-server": {
       "security": {
         "require-signed-nodes": true,
         "trusted-keys": [
           "certs/ecu_pubkey.pem"
         ]
       }
     }
   }
