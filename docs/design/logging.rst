Event Logging Internal Design
=============================

This page describes structured server event logging: the monitor extension's
``server.log`` stream and the ``.apxlog`` container written by ``apx-log``.
For command-line usage, see :doc:`../apps/apx_log`.

Logging Components
------------------

The logging path separates event production, streaming, storage, and presentation:

* ``apx_core`` supplies connection/file notifications and explicit notifications
  for validated or rejected node definitions.
* ``monitor-extension`` manages subscriptions, generates snapshots, assigns
  stream-scoped file IDs, serializes events, and queues live events. Its bounded
  queue reports overflow through ``EVENTS_DROPPED``.
* ``apx-log record`` receives event payloads and stores them in a binary
  container. It does not interpret signal data or decompress node definitions.
* ``apx-log info`` inspects container metadata, while ``apx-log print`` decodes
  event fields for text or NDJSON presentation.

Ordinary text logging is a separate surface. Server diagnostics are delivered
through the server logging API and text-log extension; a text warning does not
automatically become a structured event. Definition rejection therefore has an
explicit event notification as well as an ordinary text diagnostic.

Stream Lifecycle
----------------

The monitor extension publishes ``server.log`` as a RemoteFile v1.1
``FileStream`` over its dedicated diagnostic connection. The connection can
use UNIX-domain sockets or TCP, with optional TLS/mTLS.

Opening the file activates a subscription:

1. ``STREAM_BEGIN`` identifies the stream and supplies the format version,
   monotonic clock identifier, wall-clock anchor, and server version.
2. A snapshot describes active connections, validated nodes, published files,
   and initial values. Snapshot events carry ``SNAPSHOT``; initial file data
   also carries ``INITIAL_VALUE``.
3. ``SNAPSHOT_END`` marks the end of initial state.
4. Queued and subsequent live events are drained to the subscriber.

Events carry sequence numbers and relative monotonic timestamps. File IDs
identify files within a stream and owning connection; an address alone cannot
identify a file across different connections.

Stopping a subscription emits ``STREAM_END`` while the connection remains
usable. Connection loss can also terminate capture without delivering that
event.

Wire and Container Framing
--------------------------

On the connection, one logical RemoteFile write to ``server.log`` carries one
event. RemoteFile fragments are reassembled before the event is delivered.
NumHeader32 and the RemoteFile address header provide transport framing; the
event itself has a 28-byte common header followed by its event-specific payload.

The recorder stores event payloads, not the transport's address headers or TLS
framing. The ``.apxlog`` container consists of:

* A 64-byte header containing magic ``\0APXLOG\n``, format version ``1.0``,
  flags, capture start/end timestamps, and record count.
* Event records, each prefixed by a canonical NumHeader32 length: one byte for
  sizes up to 127 bytes and four bytes otherwise.

Normal recorder shutdown updates the container header with the end timestamp,
record count, and finalized flag. Abrupt termination can leave an unfinalized
container. Finalized status describes capture completion, not whether any
client definitions were rejected.

Event types use an application-level namespace independent of RemoteFile
command IDs. Unknown event types can be retained by the recorder even when a
consumer does not understand their payloads.

Definition Events
-----------------

Successful Definitions
~~~~~~~~~~~~~~~~~~~~~~

Live and snapshot definition-file events contain validated logical APX text
and its uncompressed size, regardless of transfer encoding.

Live events are deferred until definition validation, compilation, and
file-manager attachment succeed, then emitted as ``FILE_PUBLISH``,
``FILE_OPEN``, and ``FILE_WRITE``. Snapshots likewise include only validated
definitions. Non-definition file events retain their existing transport
semantics.

The logged source address identifies the RemoteFile, but adding the logged
logical size does not reconstruct a compressed transport address range.
Actual RemoteFile metadata and transfer bounds remain encoded and unchanged.
Neither the monitor logging path nor ``apx-log`` needs a decompression step.
Definition-transfer compression does not imply compression of ``server.log``
or ``.apxlog`` containers.

Rejected Definitions
~~~~~~~~~~~~~~~~~~~~

``NODE_DEFINITION_REJECTED`` records a failed loading attempt for either plain
or compressed transfer. It preserves:

* Sending connection ID and announced file name.
* Transfer encoding and advertised encoded/logical sizes.
* Failure stage: admission, transfer, decompression, digest verification,
  signature verification, parsing, compilation/attachment, or allocation.
* Specific detected error and intended client NACK code.

Connection ID zero is valid. Announced names are not authenticated client
identities, and resource failures do not establish that a client is malicious.

Detection and response codes can differ: an Adler-32 failure is
``APX_CHECKSUM_ERROR`` while the NACK remains ``APX_PARSE_ERROR`` for
compatibility. The event describes error detection, not successful NACK
delivery, and is emitted independently of whether the NACK can be queued or
delivered. Failure metadata remains available before node destruction;
see :doc:`apx_server` for receiver ownership and cleanup.

Rejected bytes are not emitted as normal definition-file data. Rejection
events describe live history and are not reproduced in snapshots. Ordinary
text logging also reports rejection and NACK-queuing failures.
