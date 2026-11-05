APX Server Internal Design
==========================

This page describes server-side node definition loading in ``apx_core`` and
its interaction with monitoring and logging. For daemon operation and
configuration, see :doc:`../apps/apx_server`. For embedding the server or
writing extensions, see :doc:`../guides/server_programming`.

Encoded and Logical Definitions
-------------------------------

A node definition has two representations:

* **Encoded transfer data**: the bytes published and transferred through
  RemoteFile. These may be plain APX text or a zlib-wrapped DEFLATE stream.
* **Logical definition data**: the exact uncompressed APX text used for
  hashing, signature verification, parsing, compilation, and inspection.

For a compressed publication, ``apx_file_t`` and its ``rmf_file_info_t`` retain
the encoded size and address range. Write offsets, bounds, and transfer
completion refer to encoded bytes, not the advertised uncompressed size.
The ``apx_node_data_t`` definition buffer initially holds received transfer
data and is replaced with logical text during validated node loading.

Consumers must not inspect an incomplete or unverified definition buffer.
A successfully loaded node reaches ``APX_DATA_STATE_SYNCHRONIZED`` only after
validation, compilation, and file-manager attachment succeed. Any local
C-string terminator is outside the logical size and is not hashed or signed.

Definition Admission Pipeline
-----------------------------

When a client publishes a definition file, the receiver:

1. Validates publication metadata.
2. Rejects unsigned publication immediately if signed nodes are required.
3. Checks individual size limits and reserves the server-wide temporary-buffer
   budget before allocating definition buffers or requesting the file.
4. Reassembles transfer data. Compressed definitions must start at offset
   zero, use sequential, non-overlapping fragments, and clear ``MORE_BIT``
   only at the declared encoded end.
5. Decompresses compressed data with a strict output bound, requiring a
   complete zlib stream, valid Adler-32 checksum, no trailing data, and the
   exact advertised logical size. See :doc:`../api/apx_compress`.
6. Verifies any SHA-256 digest and applicable ECDSA-P256 signature over the
   exact logical bytes.
7. Parses and compiles the definition, attaches the node to its file
   manager, releases temporary resources, and marks the definition
   synchronized.

Compression and signing are independent. Signing authenticates definition
content; it does not establish the identity of the sending client.

Buffer Ownership and Cleanup
----------------------------

The file manager owns the reassembly buffer. Node data owns the per-node
definition buffer, while the node manager owns temporary snapshots and
decompression/parsing buffers used during loading.

Definition-buffer replacement allocates the replacement before freeing the
old buffer. Failure is propagated rather than allowing a node to be admitted
without its logical definition. The parser's grown input buffer is released
after parsing instead of remaining allocated for the lifetime of the
connection. The retained logical definition belongs to node data and is
released when the node is destroyed.

Rejection closes the definition file and clears its notification handler
before deleting the incomplete node. Other file handlers owned by that node
are also detached. This prevents subsequent writes from dereferencing the
destroyed node. Receiver buffers and memory reservations are released, and a
failed parsing attempt must not poison subsequent node loading on the same
connection.

Definition Transfer Limits
--------------------------

Compile-time limits in ``apx_core/include/apx/cfg.h`` bound definition
admission before requesting or allocating transfer data:

* ``APX_MAX_FILE_SIZE``: maximum encoded and decoded size, each 16 MiB by default.
* ``APX_MAX_DEFINITION_TRANSFER_MEMORY``: server-wide temporary-buffer budget,
  128 MiB by default.
* ``APX_DEFINITION_CODEC_MEMORY_ALLOWANCE``: conservative workspace allowance
  per admitted transfer, 256 KiB by default.

The budget reserves four encoded-buffer sizes, three logical-buffer sizes,
the terminator, and codec workspace. This conservatively accounts for
receiver/parser growth, transfer snapshots, and decoded-buffer replacement.
Exhaustion rejects a publication with ``APX_BUSY_ERROR``; excessive individual
sizes use ``APX_FILE_TOO_LARGE_ERROR``. There is no waiting queue.

Each admitted node holds a reservation against a server-wide counter
protected by a dedicated mutex. Reservations are released after completion,
rejection, revocation, or connection teardown. Receiver/parser growth buffers
are included and released. Persistent node/signal storage and log queues are
not included, so this is not a limit on total server memory. These are
build-time overrides, not JSON settings.

Monitoring and Logging
----------------------

Successful Definitions
~~~~~~~~~~~~~~~~~~~~~~

Definition inspection returns validated logical text. Live and snapshot
``server.log`` definition-file events likewise contain the same uncompressed
bytes and logical size, regardless of transfer encoding.

Live definition events are deferred until loading succeeds, then emitted as
``FILE_PUBLISH``, ``FILE_OPEN``, and ``FILE_WRITE``. Snapshots include only
validated definitions. The logged source address identifies the RemoteFile,
but adding the logged logical size does not reconstruct a compressed
transport address range. Actual RemoteFile metadata remains unchanged.
Non-definition file events retain their existing transport semantics.

Neither the monitor logging path nor ``apx-log`` needs to decompress
definition transfers.

Rejected Definitions
~~~~~~~~~~~~~~~~~~~~

``NODE_DEFINITION_REJECTED`` records a failed loading attempt for either
plain or compressed transfer. It preserves the sending connection ID,
announced file name, encoding, advertised sizes, failure stage, specific
detected error, and intended client NACK code. Announced names are not
authenticated client identities.

Detection and client response codes can differ: an Adler-32 failure is
``APX_CHECKSUM_ERROR`` while the NACK remains ``APX_PARSE_ERROR`` for
compatibility. Failure notification uses metadata that remains available
before node destruction, and is emitted even if the NACK cannot be queued or
delivered. It does not claim that the client received the response.

Rejected bytes are not emitted as normal definition-file data. Rejection
events describe live history and are not reproduced in snapshots. Ordinary
text logging also reports rejection and NACK-queuing failures.

Connection event listeners are copied while holding the listener mutex and
invoked after releasing it. User callbacks must not run while an internal
mutex is held. For recording and inspecting events, see :doc:`../apps/apx_log`.
