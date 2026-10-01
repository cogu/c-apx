apx-info
========

``apx-info`` is an inspection and diagnostic utility for querying active state from a running :doc:`apx_server` daemon, such as listing connected client nodes.

Synopsis
--------

.. code-block:: text

   apx-info <command> [-c | --connect <connect_path>]
                      [-p | --port <connect_port>]
                      [--version] [-h | --help]

Commands
--------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Command
     - Description
   * - ``clients``
     - Query and display information about client nodes currently connected to the APX server.

Command-Line Options
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-c <path>``, ``--connect <path>``
     - Address or path to the ``apx-server`` socket. Accepts a UNIX domain socket path or TCP hostname/IP. Default on Linux: ``/tmp/apx.socket``. Default on Windows: ``127.0.0.1``.
   * - ``-p <port>``, ``--port <port>``
     - Target TCP port number when connecting to the server over TCP. Default: ``5000``.
   * - ``--version``
     - Display version information and exit.
   * - ``-h``, ``--help``
     - Display usage help and exit.

Usage Examples
--------------

Query Connected Clients via Default Socket
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-info clients

Query Server Over a Specific UNIX Domain Socket
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-info clients -c /tmp/apx_custom.socket

Query Server Over TCP
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-info clients -c 127.0.0.1 -p 5000
