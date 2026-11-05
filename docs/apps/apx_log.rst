apx-log
=======

``apx-log`` is a command-line utility for recording, inspecting, and viewing APX event streams. It connects to the server's :doc:`../extensions/monitor` endpoint, records server lifecycle and signal events directly into standard ``.apxlog`` binary container files, inspects container metadata, and prints formatted event records to the console in human-readable text or newline-delimited JSON (NDJSON).

Architecture & How It Works
---------------------------

``apx-log`` interfaces with the APX server daemon through the monitor extension:

1. **Diagnostic Connection**:
   ``apx-log`` connects to the dedicated diagnostic listener hosted by ``monitor-extension`` (UNIX domain socket ``/tmp/apx.socket`` or TCP endpoint, with optional TLS / mTLS).

2. **Stream Subscription (``server.log``)**:
   The monitor extension publishes the virtual file ``server.log`` as a RemoteFile v1.1 Stream File (``FileStream``). When ``apx-log record`` opens this stream, the server initiates an active streaming session:
   * **Stream Initializer**: Emits ``STREAM_BEGIN`` containing format version, monotonic clock identifier, capture start timestamp, and server version.
   * **State Snapshot**: Captures active connections, registered nodes, published files, and initial signal values, followed by ``SNAPSHOT_END``.
   * **Live Event Stream**: Transmits client connections/disconnections, node registrations, file operations, and port updates in real time.
   * **Stream Terminator**: Emits ``STREAM_END`` upon session closure.

3. **Binary Container Format (``.apxlog``)**:
   Recorded files use a standard 64-byte container header followed by NumHeader32-framed records:
   * **Header (64 bytes)**: Magic identifier ``\0APXLOG\n``, format version ``1.0``, flags (indicating finalized status), capture start timestamp (UTC nanoseconds), capture end timestamp, and total record count.
   * **Framed Records**: Each event payload delivered over ``server.log`` is preceded by a canonical NumHeader32 length prefix (1 byte for sizes up to 127 bytes, 4 bytes otherwise).
   * **Safe Finalization**: Upon receiving ``SIGINT`` (Ctrl+C), ``SIGTERM``, or server disconnection, ``apx-log`` seeks to offset 0, updates the capture end timestamp and record count, sets the finalized flag bit, and closes the file.

Synopsis
--------

.. code-block:: text

   apx-log [OPTIONS] COMMAND [ARGS...]

Global Options
--------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-h``, ``--help``
     - Display command-line usage summary and exit.
   * - ``-v``, ``--version``
     - Display version information and exit.

Commands
--------

record
~~~~~~

Streams server events directly to a ``.apxlog`` container file until interrupted:

.. code-block:: text

   apx-log record [OPTIONS] <FILE>

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-c <addr>``, ``--connect <addr>``
     - Address or path to the diagnostic endpoint. Accepts a UNIX domain socket path (e.g. ``/tmp/apx.socket``) or TCP hostname/IP (e.g. ``127.0.0.1:5080``). Default on Linux: ``/tmp/apx.socket``. Default on Windows: ``127.0.0.1:5000``.
   * - ``--tls``
     - Enable TLS transport for encrypted communication.
   * - ``--ca-cert <file>``
     - Path to the trusted CA certificate (PEM) used to authenticate the APX server.
   * - ``--client-cert <file>``
     - Path to the client public certificate (PEM) for mutual TLS (mTLS) authentication.
   * - ``--client-key <file>``
     - Path to the client private key (PEM) for mutual TLS (mTLS) authentication.

info
~~~~

Inspects and displays header metadata and summary statistics for a ``.apxlog`` container:

.. code-block:: text

   apx-log info [OPTIONS] <FILE>

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``--json``
     - Emit container metadata in structured JSON format.

Output includes format version, finalized status, capture start/end timestamps (ISO 8601 UTC and unix nanoseconds), total duration, record count, and file size on disk.

Definition Content and Rejections
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Live and snapshot ``.apx`` file events contain validated, uncompressed APX
text and its logical size, regardless of transfer encoding. ``apx-log`` does
not decompress node transfers or compress the ``.apxlog`` container.

``NODE_DEFINITION_REJECTED`` records the sending connection, announced file
name, transfer encoding and sizes, failure stage, specific detected error,
and intended client NACK code. ``print`` displays these fields as text;
``print --json`` exposes ``transfer_encoding``, ``failure_stage``,
``error_code``, and ``response_code``. Connection ID zero is valid and is
included in JSON output. The response code does not prove NACK delivery.
Rejected payload bytes are not recorded as normal definition-file data.

print
~~~~~

Sequentially parses and decodes each event record from a ``.apxlog`` container:

.. code-block:: text

   apx-log print [OPTIONS] <FILE>

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``--json``
     - Output each event record as an NDJSON (newline-delimited JSON) object.

Examples
--------

Recording an Event Log
~~~~~~~~~~~~~~~~~~~~~~

Connect to a local server instance and record events to ``capture.apxlog``:

.. code-block:: bash

   $ apx-log record -c /tmp/apx.socket capture.apxlog
   Recording events to capture.apxlog (Ctrl+C to stop)...
   ^C
   Recording stopped. Finalized 42 records to capture.apxlog

Inspecting Container Metadata
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

View container summary in human-readable text:

.. code-block:: bash

   $ apx-log info capture.apxlog
   Container:       capture.apxlog
   Format Version:  1.0
   Status:          finalized
   Capture Start:   2026-11-01T17:30:00.123Z (1793554200123000000 ns)
   Capture End:     2026-11-01T17:30:15.654Z (1793554215654000000 ns)
   Duration:        15.531s (15531000000 ns)
   Record Count:    42
   File Size:       3412 bytes

View container summary as JSON:

.. code-block:: bash

   $ apx-log info --json capture.apxlog
   {
     "file": "capture.apxlog",
     "format_major": 1,
     "format_minor": 0,
     "status": "finalized",
     "finalized": true,
     "capture_start_unix_ns": 1793554200123000000,
     "capture_end_unix_ns": 1793554215654000000,
     "duration_ns": 15531000000,
     "record_count": 42,
     "file_size": 3412
   }

Displaying Formatted Events
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Display sequential events in human-readable format:

.. code-block:: bash

   $ apx-log print capture.apxlog
   [+ 0.000000s] [seq=0] STREAM_BEGIN v1.0 wall=1793554200123000000 server=0.3.15 stream_id=8a3f...
   [+ 0.001200s] [seq=1] [SNAP] CONNECTION_CREATE conn=1 type=1 state=1 tag="" endpoint="127.0.0.1:41234"
   [+ 0.001350s] [seq=2] [SNAP] FILE_PUBLISH conn=1 address=0x00000000 file="EngineNode.out" size=4
   [+ 0.002000s] [seq=3] SNAPSHOT_END
   [+ 0.150000s] [seq=4] FILE_WRITE conn=1 file_id=0 offset=0 size=4 [01 00 00 00]

Emit event stream as NDJSON lines:

.. code-block:: bash

   $ apx-log print --json capture.apxlog
   {"event":"STREAM_BEGIN","seq":0,"timestamp_ns":0,"format_major":1,"format_minor":0,"clock_id":1,"server_version":"0.3.15"}
   {"event":"CONNECTION_CREATE","seq":1,"timestamp_ns":1200000,"flags":["SNAPSHOT"],"connection_id":1,"type":1,"state":1,"endpoint":"127.0.0.1:41234"}
   {"event":"FILE_PUBLISH","seq":2,"timestamp_ns":1350000,"flags":["SNAPSHOT"],"connection_id":1,"file_address":0,"file_size":4,"file_name":"EngineNode.out"}
   {"event":"SNAPSHOT_END","seq":3,"timestamp_ns":2000000}
   {"event":"FILE_WRITE","seq":4,"timestamp_ns":150000000,"connection_id":1,"file_id":0,"offset":0,"size":4,"data_hex":"01 00 00 00"}
