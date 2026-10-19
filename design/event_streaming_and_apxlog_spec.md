# APX Event Stream and `.apxlog` Format

## 1. Scope

This document specifies:

* the event stream published by `apx-server` as the RemoteFile stream file
  `server.log`;
* the ordering, snapshot, overflow, and timestamp rules for that stream;
* the append-only `.apxlog` container written by `apxlog record`; and
* the minimum behavior required from `apxlog replay`.

The event protocol is an application protocol carried by RemoteFile v1.1. It
does not add a RemoteFile command and it does not add another length field to
the wire payload.

The normative lower-layer specifications are:

* RemoteFile v1.1;
* NumHeader32; and
* the APX IDL and VM versions advertised by the recorded node definitions.

## 2. Protocol layering

On a stream transport, one complete event has the following framing:

```text
+----------------------+---------------------------------------+
| NumHeader32          | RemoteFile write                      |
| RMFP payload length  | AddressHeader + one event payload     |
+----------------------+---------------------------------------+
```

The NumHeader32 value already defines the RMFP message boundary. The
AddressHeader identifies `server.log`. Therefore an event payload MUST NOT
contain `record_len`, `payload_len`, or another application framing field.

The following rules are normative:

1. One logical RemoteFile write to `server.log` carries exactly one event.
2. Multiple events MUST NOT be packed into one logical write.
3. One event MUST NOT be split into independently delivered stream writes.
4. RMFP MAY fragment one logical write with `MORE_BIT`. The RemoteFile receiver
   reassembles those fragments and delivers the event only after the fragment
   with `MORE_BIT = 0`.
5. Every write starts at the advertised start address of `server.log`.
6. The event size is the reassembled RemoteFile data size. The advertised
   `FileSize` of `server.log` is the maximum accepted event size.

These rules preserve RemoteFile message boundaries all the way to the event
decoder. A receiver never scans a byte stream to find an event boundary.

## 3. Stream publication and lifetime

### 3.1 File announcement

After accepting a diagnostic connection, the server publishes:

| File name | RemoteFile type | Direction | Purpose |
|:---|:---|:---|:---|
| `server.log` | `FileStream` (`5`) | Server to client | Version 1 event stream |

The file is published with `DigestType = NoDigest`. `FileSize` is a server
limit and MUST be at least large enough for the common event header plus one
byte. A recommended default is 1 MiB.

Publishing the file advertises capability only. The server MUST NOT create a
snapshot or enable event serialization until the client sends
`RMF_CMD_FILE_OPEN` for `server.log`.

### 3.2 Start and stop

After the open request, the server sends:

1. one `STREAM_BEGIN` event;
2. a coherent snapshot, with `SNAPSHOT` set on every snapshot event;
3. one `SNAPSHOT_END` event; and
4. live events in sequence order.

`RMF_CMD_FILE_CLOSE`, file revocation, or connection loss ends the
subscription. A graceful server shutdown SHOULD send `STREAM_END` before
closing the file.

Reopening `server.log` starts a new stream with a new `stream_id`, a new
snapshot, and sequence number zero. Sequence numbers never continue across
opens or reconnects.

## 4. Primitive encoding

All multi-byte integers use little-endian byte order. Structures are packed;
there is no implicit alignment or padding.

The following variable-sized values are used:

| Type | Encoding |
|:---|:---|
| `String16` | `U16 byte_count`, followed by exactly that many UTF-8 bytes |
| `Bytes` | All bytes remaining in the enclosing RemoteFile write |

`String16` has no terminating NUL. A sender MUST emit valid UTF-8. A receiver
MUST reject a value whose declared size exceeds the remaining message.

## 5. Common event header

Every event begins with this 28-byte header:

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `event_type` | `U16` | Event identifier from Section 6 |
| 2 | `event_flags` | `U16` | Common and event-specific flags |
| 4 | `sequence` | `U64` | Stream sequence number |
| 12 | `timestamp_ns` | `U64` | Nanoseconds since the stream monotonic origin |
| 20 | `connection_id` | `U32` | Server connection ID, or `0` when not applicable |
| 24 | `object_id` | `U32` | Event-specific node, file, or subscription-local ID |
| 28 | `payload` | bytes | Event-specific fields through the RMFP message end |

### 5.1 Common flags

| Bit | Name | Meaning |
|:---:|:---|:---|
| 0 | `SNAPSHOT` | The event describes state captured during initial synchronization |
| 1 | `INITIAL_VALUE` | A `FILE_WRITE` contains state at snapshot time rather than an observed write |
| 2-15 | Reserved | Sender writes zero; receiver ignores |

### 5.2 Sequence numbers

`STREAM_BEGIN` has sequence `0`. The server allocates the next sequence number
before enqueueing each event. Consequently, a queue overflow creates a visible
gap even if an `EVENTS_DROPPED` event cannot be emitted immediately.

A receiver MUST accept unknown event types and skip them using the enclosing
RemoteFile message boundary. It MUST NOT infer a missing event merely because
it does not understand an event type.

### 5.3 Time

`timestamp_ns` uses a monotonic clock and is relative to the origin reported by
`STREAM_BEGIN`. It is not nanoseconds since the Unix epoch. This prevents wall
clock adjustments from changing replay order or delays.

`STREAM_BEGIN` supplies a wall-clock anchor for correlation with external logs.
The anchor is informative; sequence order is authoritative when two events
have equal timestamps.

## 6. Event types

| Value | Name | `connection_id` | `object_id` |
|:---:|:---|:---|:---|
| `0x0001` | `STREAM_BEGIN` | `0` | `0` |
| `0x0002` | `SNAPSHOT_END` | `0` | `0` |
| `0x0010` | `CONNECTION_CREATE` | Connection | `0` |
| `0x0011` | `CONNECTION_CLOSE` | Connection | `0` |
| `0x0020` | `NODE_ATTACH` | Owning connection | Node ID |
| `0x0021` | `NODE_DETACH` | Owning connection | Node ID |
| `0x0030` | `FILE_PUBLISH` | Owning connection | File ID |
| `0x0031` | `FILE_REVOKE` | Owning connection | File ID |
| `0x0032` | `FILE_OPEN` | Owning connection | File ID |
| `0x0033` | `FILE_CLOSE` | Owning connection | File ID |
| `0x0034` | `FILE_WRITE` | Owning connection | File ID |
| `0x00FE` | `EVENTS_DROPPED` | `0` | `0` |
| `0x00FF` | `STREAM_END` | `0` | `0` |

IDs are scoped to one stream. A `file_id` is not a RemoteFile address: file
addresses can be identical on different connections. The tuple
`(stream_id, connection_id, file_id)` uniquely identifies a recorded file.

### 6.1 `STREAM_BEGIN`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `format_major` | `U16` | `1` |
| `format_minor` | `U16` | `0` |
| `clock_id` | `U16` | `1` = monotonic |
| `reserved` | `U16` | Zero |
| `wall_clock_unix_ns` | `U64` | Unix time corresponding to stream time zero |
| `stream_id` | `U8[16]` | Random 128-bit stream identifier |
| `server_version` | `String16` | Server software version |

The event has timestamp zero and no flags.

### 6.2 `SNAPSHOT_END`

Payload: empty.

All later events are live events and MUST have `SNAPSHOT` clear.

### 6.3 `CONNECTION_CREATE`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `connection_type` | `U8` | APX connection type |
| `connection_state` | `U8` | APX connection state |
| `reserved` | `U16` | Zero |
| `tag` | `String16` | Connection tag |
| `endpoint` | `String16` | Peer address, socket path, or empty string |

### 6.4 `CONNECTION_CLOSE`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `reason` | `U32` | `apx_error_t` value, or `APX_NO_ERROR` for an orderly close |

### 6.5 `NODE_ATTACH`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `node_name` | `String16` | APX node name |

`node_id` is allocated by the recorder source and remains valid until
`NODE_DETACH`.

### 6.6 `NODE_DETACH`

Payload: empty.

### 6.7 `FILE_PUBLISH`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `remote_address` | `U32` | Address in the owning connection's RemoteFile map |
| `file_size` | `U32` | Published RemoteFile size |
| `file_type` | `U16` | RemoteFile file type |
| `digest_type` | `U16` | RemoteFile digest type |
| `digest_data` | `U8[32]` | Digest, zero-padded as in `RMF_CMD_FILE_INFO` |
| `file_name` | `String16` | Published file name |

The event intentionally mirrors stable fields from `RMF_CMD_FILE_INFO`, but
uses a stream-local `file_id` so later records do not depend on a reusable
address.

### 6.8 `FILE_REVOKE`, `FILE_OPEN`, and `FILE_CLOSE`

Payload: empty.

### 6.9 `FILE_WRITE`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `file_offset` | `U32` | Offset from the published file start address |
| `data` | `Bytes` | Bytes written, extending to the end of the event |

There is no `data_len`; the enclosing RMFP message determines it.

If one observed write would exceed the advertised `server.log` `FileSize`, the
server emits consecutive `FILE_WRITE` events with increasing offsets. Each
event represents a complete write range and can be replayed independently.

Definition text is not a special event. A snapshot is self-contained because
the `.apx` file is represented by `FILE_PUBLISH` followed by one or more
`FILE_WRITE` events containing its bytes. The same rule applies to `.in`,
`.out`, `.cin`, and `.cout` files.

### 6.10 `EVENTS_DROPPED`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `first_sequence` | `U64` | First sequence number not delivered |
| `dropped_count` | `U64` | Number of consecutive dropped events |

The first event accepted after an overflow window MUST be
`EVENTS_DROPPED`. Its own `sequence` follows the dropped range. Receivers MUST
also detect sequence gaps because a connection can fail before this record is
sent.

### 6.11 `STREAM_END`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `reason` | `U32` | `apx_error_t` value, or `APX_NO_ERROR` |

## 7. Snapshot and live handoff

The snapshot must not lose events that occur while it is being serialized.
The server uses this ordering:

1. Register the subscriber with the event source and allocate sequence zero.
2. Capture a consistent topology/file-state view.
3. Queue new live events that occur after that view.
4. Emit `STREAM_BEGIN`, snapshot events, and `SNAPSHOT_END`.
5. Drain the queued live events in sequence order.

Snapshot events describe the captured view and have `SNAPSHOT` set. They do
not claim to be the historical events that originally created that state.

If the live queue overflows during the snapshot, normal drop reporting applies
after `SNAPSHOT_END`. The server MUST NOT block the APX routing path while it
waits for a recorder.

## 8. Producer isolation and overflow

Event capture runs in APX server callback paths, so it MUST be bounded and
non-blocking:

* capture copies only the metadata and bytes needed to reproduce the event;
* no socket or disk I/O occurs in the producer callback;
* a dedicated consumer serializes and transmits events;
* a full queue drops newly produced events and advances the sequence counter;
* registered user callbacks are never invoked while an internal server mutex is
  held.

The implementation may use a lock-free queue or another bounded queue whose
producer operation cannot wait. Lock-free behavior is an implementation
choice, not a wire-protocol requirement.

Drop accounting is per subscriber because each subscriber has an independent
transport and queue.

## 9. `.apxlog` container

RMFP framing ends at the recorder. A regular file has no RemoteFile envelope,
so the container adds framing for sequential recovery. It reuses NumHeader32
instead of embedding `record_len` in the event itself.

```text
+----------------------+-------------------------------------------+
| 64-byte file header  |                                           |
+----------------------+-------------------------------------------+
| NumHeader32          | Exact event payload received on server.log|
+----------------------+-------------------------------------------+
| NumHeader32          | Exact event payload received on server.log|
+----------------------+-------------------------------------------+
| ...                                                              |
+------------------------------------------------------------------+
| Optional index trailer                                           |
+------------------------------------------------------------------+
```

### 9.1 File header

All integers are little-endian.

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `magic` | `U8[8]` | `00 41 50 58 4c 4f 47 0a` (`"\0APXLOG\n"`) |
| 8 | `format_major` | `U16` | `1` |
| 10 | `format_minor` | `U16` | `0` |
| 12 | `header_size` | `U32` | `64` |
| 16 | `flags` | `U32` | Bit 0: finalized; bit 1: index present |
| 20 | `reserved` | `U32` | Zero |
| 24 | `capture_start_unix_ns` | `U64` | Copied from `STREAM_BEGIN` |
| 32 | `capture_end_unix_ns` | `U64` | Zero until finalized |
| 40 | `record_count` | `U64` | Zero until finalized |
| 48 | `index_offset` | `U64` | Zero when no index is present |
| 56 | `data_offset` | `U64` | `64` |

Each record after the header is:

1. a canonical NumHeader32 containing the event payload size; then
2. the exact event bytes delivered by the reassembled `server.log` write.

Writers MUST use the one-byte NumHeader32 form for sizes up to 127 and the
four-byte form otherwise. Readers MUST reject non-canonical or out-of-range
headers.

### 9.2 Optional index trailer

The trailer begins at `index_offset`:

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `magic` | `U8[8]` | `00 41 50 58 49 44 58 0a` (`"\0APXIDX\n"`) |
| 8 | `format_major` | `U16` | `1` |
| 10 | `entry_size` | `U16` | `24` |
| 12 | `reserved` | `U32` | Zero |
| 16 | `entry_count` | `U64` | Number of following entries |

Each 24-byte entry contains:

| Field | Type | Description |
|:---|:---:|:---|
| `sequence` | `U64` | Indexed event sequence |
| `timestamp_ns` | `U64` | Event timestamp |
| `file_offset` | `U64` | Offset of that event's NumHeader32 |

An index may be sparse. Entries MUST be ordered by `(timestamp_ns, sequence)`.

### 9.3 Finalization and crash recovery

On clean shutdown, the writer:

1. flushes complete event records;
2. optionally appends and flushes the index;
3. updates end time, record count, index offset, and flags in the header; and
4. flushes the header update.

If `finalized` is clear, a reader scans NumHeader32-framed records from
`data_offset` to EOF. An incomplete final NumHeader or payload is ignored.
Earlier complete records remain valid.

If `index present` is set, event scanning stops at `index_offset`. A reader
MUST ignore an index whose bounds, magic, entry size, or ordering is invalid and
fall back to sequential scanning.

## 10. Replay requirements

`apxlog replay` connects to the operational APX endpoint as a normal APX
client. It does not inject events through the diagnostic endpoint.

Before timed playback it MUST:

1. process the snapshot through `SNAPSHOT_END`;
2. reconstruct node definitions and published data files;
3. apply include/exclude filters; and
4. publish the selected simulated nodes using normal APX files.

For live records, replay delay is based on monotonic `timestamp_ns` differences:

```text
target = playback_start + (event_time - first_replayed_time) / rate
```

Sequence order breaks timestamp ties. Replay MUST report `EVENTS_DROPPED`,
unexplained sequence gaps, missing definitions, and writes referencing unknown
files. It MUST NOT silently present an incomplete recording as lossless.

Connection and file lifecycle events belonging only to excluded nodes are not
replayed. Writes to `.out` files are reproduced through normal node/file APIs;
recorded `.in` values are observational and are not injected as providers.

## 11. Validation requirements

Implementations require tests for:

* one event per complete RemoteFile write, including `MORE_BIT` reassembly;
* rejection of packed, truncated, oversized, and malformed events;
* snapshot/live handoff while files are changing;
* queue overflow, sequence gaps, and `EVENTS_DROPPED`;
* multiple connections using the same RemoteFile address;
* `.apxlog` recovery after every possible truncated NumHeader/payload byte;
* corrupt index fallback;
* monotonic replay ordering across wall-clock changes; and
* replay with node inclusion and exclusion filters.
