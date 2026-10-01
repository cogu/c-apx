apx-control
===========

``apx-control`` is a command-line client designed to interact with the JSON message server hosted by an :doc:`apx_node` instance. It allows developers and automated test harnesses to write signal values directly into a running node's provide-ports.

Synopsis
--------

.. code-block:: text

   apx-control [-i <json_file>]
               [-c | --connect <connect_path>]
               [-p | --port <connect_port>]
               [--version] [-h | --help]
               [<name> <value>]

Command-Line Options
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-c <path>``, ``--connect <path>``
     - Target address of the ``apx-node`` control socket. Accepts a UNIX domain socket path (e.g. ``/tmp/apx_node.socket``) or a TCP hostname/IP address. Default on Linux: ``/tmp/apx_node.socket``. Default on Windows: ``127.0.0.1``.
   * - ``-p <port>``, ``--port <port>``
     - Target TCP port number when connecting to a remote node over TCP. Default: ``5100``.
   * - ``-i <json_file>``
     - Read the JSON signal payload from a file on disk instead of positional command-line arguments.
   * - ``<name> <value>``
     - Positional arguments specifying the signal name and value to publish. Values can be numeric (integers or floating point) or strings.
   * - ``--version``
     - Display version information and exit.
   * - ``-h``, ``--help``
     - Display usage help and exit.

JSON Message Format
-------------------

``apx-control`` sends JSON objects to the ``apx-node`` process over the connected socket:

.. code-block:: json

   {
     "VehicleSpeed": 65.5,
     "GearPosition": 3
   }

When setting signals via positional arguments:

.. code-block:: bash

   apx-control VehicleSpeed 65.5

The utility serializes the argument pair into ``{"VehicleSpeed": 65.5}`` before transmitting it to the node.

Usage Examples
--------------

1. Set a Single Numeric Signal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-control VehicleSpeed 80

2. Connect to a Specific Node Socket
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-control -c /tmp/node_sensor.socket Temperature 24.5

3. Connect Over TCP
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-control -c 192.168.1.100 -p 5100 EngineRPM 3200

4. Inject Batch Signals from a File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create a file named ``signals.json``:

.. code-block:: json

   {
     "VehicleSpeed": 95,
     "EngineState": 2,
     "BrakePedalPressed": false
   }

Apply the updates in a single transaction:

.. code-block:: bash

   apx-control -c /tmp/apx_node.socket -i signals.json
