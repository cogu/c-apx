apx-log
=======

``apx-log`` is a command-line utility for recording, inspecting, and viewing APX event streams. It connects to the server's :doc:`../extensions/monitor` endpoint, records server lifecycle and signal events directly into standard ``.apxlog`` binary container files, inspects container metadata, and prints formatted event records to the console in human-readable text or newline-delimited JSON (NDJSON).

Recording Overview
------------------

A recording starts with a snapshot of current server state, followed by live
events. Stop recording with Ctrl+C to finalize the capture, then use ``info``
for a summary or ``print`` to inspect its events. For implementation and file
format details, see :doc:`../design/logging`.

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

Error Handling
--------------

Command Failures
~~~~~~~~~~~~~~~~

Errors opening files or connecting to the diagnostic endpoint are reported
on stderr and return a non-zero exit status. Use ``apx-log info`` to inspect
whether a capture was finalized; ``unfinalized / crashed`` means recording
did not complete normal finalization.

Rejected Node Definitions
~~~~~~~~~~~~~~~~~~~~~~~~~

A server-side node rejection is a recorded event, not necessarily a failure
of the recording command. Use ``apx-log print`` to inspect
``NODE_DEFINITION_REJECTED`` entries, or ``print --json`` for the
``connection_id``, ``file_name``, ``transfer_encoding``, ``failure_stage``,
``error_code``, and ``response_code`` fields.

The failure stage distinguishes decompression errors from later signature or
parsing failures. ``response_code`` is the intended client NACK code; it does
not prove that the client received that response. Finalized captures can
contain rejection events, and rejections that occurred before recording
started are not included in the initial snapshot.

For event-generation and definition-content semantics, see
:doc:`../design/logging`.
