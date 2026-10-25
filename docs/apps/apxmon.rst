apxmon
======

``apxmon`` is an interactive diagnostic and inspection utility for querying active state, routing tables, connected clients, and live signal values from a running :doc:`apx_server` daemon. It connects to the server's :doc:`../extensions/monitor` and provides both human-readable tabular output and machine-readable JSON/NDJSON streams.

Architecture & How It Works
---------------------------

``apxmon`` interacts with ``apx-server`` via a dedicated, secure diagnostic session:

1. **Dedicated Diagnostic Endpoint**:
   Rather than connecting through the operational signal port (default ``5000``), ``apxmon`` connects to the dedicated diagnostic listener hosted by ``monitor-extension`` (UNIX domain socket ``/tmp/apx.socket`` or TCP port ``5080``, with optional mutual TLS).

2. **RemoteFile v1.1 Stream Files**:
   The diagnostic protocol is layered entirely on RemoteFile v1.1 Stream Files (``FileStream``), avoiding custom socket framing:

   * **``apx.diag.v1.request``**: Published by ``apxmon`` to transmit diagnostic requests to the server.
   * **``apx.diag.v1.response``**: Published by the server to return query results and command acknowledgments.
   * **``apx.diag.v1.notify``**: Published by the server for asynchronous port value notifications and invalidation events.

3. **Multiplexed Requests and Zero Idle Overhead**:
   Requests and responses carry 32-bit correlation IDs (``request_id``), allowing requests to be matched deterministically. The server maintains zero shadow copies or duplicate caches while idle; queries take immediate, read-only snapshots directly from internal server structures.

4. **Dynamic Node Definition Transfer**:
   When requesting node definitions with ``get-def``, the server dynamically provisions a temporary APX file (``<NodeName>.apx``) and streams it directly across the RemoteFile channel.

Synopsis
--------

.. code-block:: text

   apxmon [OPTIONS] COMMAND [ARGS...]

Global Options
--------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-c <addr>``, ``--connect <addr>``
     - Address or path to the diagnostic endpoint. Accepts a UNIX domain socket path (e.g. ``/tmp/apx.socket`` or ``/run/apx/apx_monitor.socket``) or TCP hostname/IP (e.g. ``127.0.0.1:5080``). Default on Linux: ``/tmp/apx.socket``. Default on Windows: ``127.0.0.1:5000``.
   * - ``--tls``
     - Enable TLS transport for encrypted communication with the monitor endpoint.
   * - ``--ca-cert <file>``
     - Path to the trusted CA certificate (PEM) used to authenticate the APX server.
   * - ``--client-cert <file>``
     - Path to the client public certificate (PEM) for mutual TLS (mTLS) authentication.
   * - ``--client-key <file>``
     - Path to the client private key (PEM) for mutual TLS (mTLS) authentication.
   * - ``--json``
     - Emit structured JSON output (or newline-delimited JSON for streaming commands).
   * - ``-h``, ``--help``
     - Display command-line usage summary and exit.
   * - ``-v``, ``--version``
     - Display version information and exit.

Commands
--------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Command
     - Description
   * - ``info``
     - Display server summary information, including server version, protocol version, start timestamp, uptime, and active connection/node/route counts.
   * - ``clients``
     - List all client and monitor connections currently established with the APX server.
   * - ``nodes``
     - List all registered APX nodes, their connection IDs, and provide/require port counts.
   * - ``routing [--unconnected]``
     - List active signal routes connecting provide-ports to require-ports. When ``--unconnected`` is passed, displays unconnected require-ports (missing providers) and provide-ports (missing consumers).
   * - ``get-def <node_name>``
     - Fetch and display the full APX IDL specification text (``.apx``) for the specified node.
   * - ``watch <selector...>``
     - Subscribe to live signal changes on one or more port selectors (e.g. ``NodeName:PortName`` or ``NodeID:PortID``). Streams updates continuously until interrupted.
   * - ``top``
     - Interactive terminal dashboard continuously displaying live server uptime, active connections, and registered node metrics at 1-second intervals.

Command Details
---------------

info
~~~~

Queries overall operational status from the APX server:

.. code-block:: text

   Server Version:     0.3.0
   Protocol Minor:     0
   Uptime:             1h 12m 34s (4354000 ms)
   Active Connections: 3
   Active Nodes:       2
   Active Routes:      14

In ``--json`` mode, returns a JSON object with numeric timestamps, millisecond uptimes, and resource counters.

clients
~~~~~~~

Displays a tabular summary of all active client sessions:

.. code-block:: text

   ID    TYPE       STATE        TAG                  ENDPOINT
   ----------------------------------------------------------------------
   1     CLIENT     CONNECTED    EngineController     127.0.0.1:48212
   2     CLIENT     CONNECTED    DashboardNode        127.0.0.1:48218
   3     MONITOR    CONNECTED    apxmon               /tmp/apx.socket

nodes
~~~~~

Lists each attached APX node along with its port statistics:

.. code-block:: text

   ID    CONN_ID  PROVIDE_PORTS  REQUIRE_PORTS  NODE_NAME
   ----------------------------------------------------------------------
   1     1        8              2              EngineController
   2     2        0              6              DashboardNode

routing
~~~~~~~

Inspects the server's signal routing table. By default, active routes between provider ports and consumer ports are displayed:

.. code-block:: text

   PORT_NAME                PROVIDER (NODE:PORT) CONSUMER (NODE:PORT)
   ----------------------------------------------------------------------
   VehicleSpeed             1:0                  2:0
   EngineSpeed              1:1                  2:1
   CoolantTemp              1:2                  2:2

When invoked with ``--unconnected``, the command reveals ports that are not currently routed:

.. code-block:: text

   NODE_ID  PORT_ID  DIRECTION  REASON         PORT_NAME
   ----------------------------------------------------------------------
   1        3        PROVIDE    NO_CONSUMER    BatteryVoltage
   2        5        REQUIRE    NO_PROVIDER    AmbientAirTemp

get-def
~~~~~~~

Retrieves the APX Interface Definition Language (IDL) text for any registered node:

.. code-block:: bash

   apxmon get-def EngineController

Outputs the complete text definition directly to standard output:

.. code-block:: text

   APX/1.3
   N"EngineController"
   P"VehicleSpeed"S:=65535
   P"EngineSpeed"S:=65535
   P"CoolantTemp"C:=255

When combined with ``--json``, the definition is wrapped inside a JSON object: ``{"node_name": "EngineController", "definition": "..."}``.

watch
~~~~~

Subscribes to live port updates on specified node ports using the ``apx.diag.v1.notify`` event stream:

.. code-block:: bash

   apxmon watch EngineController:VehicleSpeed EngineController:EngineSpeed

Each value change produces an immediate timestamped event line:

.. code-block:: text

   Subscribed (id=1) to 2 selector(s). Press Ctrl+C to stop.
     Selector #0 -> Node 1, Port 0 (size=2, status=0)
     Selector #1 -> Node 1, Port 1 (size=2, status=0)
   [128392100 ns] Node 1, Port 0: (len=2) 0x2710
   [158910400 ns] Node 1, Port 1: (len=2) 0x0BB8

In ``--json`` mode, events are streamed as Newline-Delimited JSON (NDJSON) objects containing event type, timestamp, sequence numbers, and hexadecimal payload values:

.. code-block:: json

   {"subscription_id":1,"event_type":1,"type":"PORT_VALUE","sequence":0,"timestamp_ns":128392100,"node_id":1,"port_id":0,"value_hex":"2710"}

top
~~~

Launches an interactive live monitoring dashboard. The display refreshes once per second, presenting real-time server uptime, active connections, and attached nodes. Press ``Ctrl+C`` to stop.

Usage Examples
--------------

1. Query Server Summary
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apxmon info

2. Connect via Custom UNIX Domain Socket
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apxmon -c /run/apx/apx_monitor.socket clients

3. Connect Over TCP with JSON Output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apxmon -c 192.168.1.100:5080 --json nodes

4. Inspect Unconnected Ports
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apxmon routing --unconnected

5. Live Watch Signal Changes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apxmon watch EngineController:VehicleSpeed

6. Connect with Mutual TLS (mTLS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apxmon -c 127.0.0.1:5020 --tls \
     --ca-cert /etc/apx/certs/ca.pem \
     --client-cert /etc/apx/certs/monitor_client.pem \
     --client-key /etc/apx/certs/monitor_client.key \
     info
