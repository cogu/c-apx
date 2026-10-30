# APX Diagnostic Protocol and `apxmon`

## 1. Scope

This document specifies the version 1 diagnostic application protocol used by
`apxmon` and the APX server monitor extension.

The protocol runs entirely over RemoteFile v1.1. It does not define a socket
frame, a RemoteFile command, or a second message-size header. RemoteFile
provides transport framing, file discovery, access intent, fragmentation, and
large-file transfer.

The security and listener configuration are specified separately in
`securing_monitor_extension.md`.

## 2. Design principles

1. **RemoteFile owns framing.** One logical RemoteFile write carries one
   diagnostic message.
2. **Files define direction and message class.** A request does not need a
   `msg_type` field because requests, responses, and notifications use
   different files.
3. **No redundant length.** Message size is the reassembled RemoteFile data
   size derived from NumHeader and AddressHeader.
4. **Control data stays small.** Large node definitions are transferred as
   ordinary RemoteFile files, not embedded in a response.
5. **Queries are snapshot-consistent.** Potentially large lists use bounded
   pages tied to a server snapshot.
6. **Breaking changes are discoverable.** The major protocol version is in the
   published file names.

## 3. RemoteFile channels

### 3.1 Files

| File name | Publisher | Reader | RemoteFile type | Contents |
|:---|:---|:---|:---|:---|
| `apx.diag.v1.request` | Client | Server | `FileStream` (`5`) | Requests |
| `apx.diag.v1.response` | Server | Client | `FileStream` (`5`) | Responses |
| `apx.diag.v1.notify` | Server | Client | `FileStream` (`5`) | Subscription notifications |

`FileSize` is the maximum logical message size accepted on that channel. The
recommended minimum is 64 KiB. A peer MUST reject a message larger than the
advertised size.

A server supporting another incompatible major version publishes another set
of versioned files. Minor-compatible additions use new opcodes, flags, or
fields explicitly allowed by this specification.

### 3.2 Session startup

After the RMFP 1.1 handshake:

1. The server publishes `apx.diag.v1.response` and `apx.diag.v1.notify`.
2. The client publishes `apx.diag.v1.request`.
3. The server opens `apx.diag.v1.request`.
4. The client opens `apx.diag.v1.response`.
5. The client may open `apx.diag.v1.notify` before creating a subscription.
6. Requests may begin after steps 3 and 4 are complete.

The server MUST NOT send responses until the response file is open. It MUST
reject subscription requests with `APX_INVALID_STATE_ERROR` unless the
notification file is open.

Closing the request or response file cancels all outstanding requests. Closing
the notification file cancels all subscriptions for that diagnostic
connection.

### 3.3 Message boundaries

On TCP or a UNIX stream socket:

```text
+----------------------+----------------------------------------------+
| NumHeader32          | RemoteFile write                             |
| RMFP payload length  | AddressHeader + one diagnostic message       |
+----------------------+----------------------------------------------+
```

The following rules are normative:

* one logical write to a diagnostic file contains exactly one message;
* messages are never concatenated in one write;
* RMFP `MORE_BIT` fragments are reassembled before diagnostic decoding;
* every write starts at the advertised stream-file address; and
* the message ends with the logical RemoteFile write.

A decoder therefore receives `(channel, bytes, size)` and never searches for a
delimiter or an application-level length.

## 4. Primitive encoding

All multi-byte integers are little-endian. Structures are packed and have no
implicit padding.

| Type | Encoding |
|:---|:---|
| `String16` | `U16 byte_count` followed by that many UTF-8 bytes |
| `Bytes` | All bytes remaining in the enclosing message |

Strings have no NUL terminator. Senders MUST produce valid UTF-8. Unless a
payload explicitly permits extensions, a receiver MUST consume the payload
exactly and reject trailing bytes.

## 5. Message envelopes

The channel identifies which envelope applies. There is no `msg_len`,
`msg_type`, or repeated protocol version.

### 5.1 Request

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `request_id` | `U32` | Client-selected, non-zero correlation ID |
| 4 | `opcode` | `U16` | Command from Section 7 |
| 6 | `flags` | `U16` | Opcode flags; zero unless specified |
| 8 | `payload` | bytes | Opcode request payload through message end |

At most one request with a given `request_id` may be active. An ID may be
reused after its response has been received.

### 5.2 Response

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `request_id` | `U32` | Copied from the request |
| 4 | `opcode` | `U16` | Copied from the request |
| 6 | `flags` | `U16` | Zero in version 1 |
| 8 | `status` | `U32` | `apx_error_t`; zero is success |
| 12 | `payload` | bytes | Opcode response payload through message end |

Every syntactically readable request receives exactly one response. Responses
may arrive in a different order than requests. A failed response has an empty
payload unless the opcode explicitly defines error details.

### 5.3 Notification

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `subscription_id` | `U32` | Subscription returned by `SUBSCRIBE_PORTS` |
| 4 | `event_type` | `U16` | Notification type from Section 8 |
| 6 | `flags` | `U16` | Notification flags |
| 8 | `sequence` | `U64` | Notification-channel sequence |
| 16 | `timestamp_ns` | `U64` | Nanoseconds since the server-running transition |
| 24 | `payload` | bytes | Event payload through message end |

Sequence numbers are global to the notification file, begin at zero when the
file is opened, and increase before enqueue. Gaps indicate dropped
notifications.

## 6. Errors and malformed messages

`status` uses values from `apx/error.h`. Common values are:

| Status | Meaning in this protocol |
|:---|:---|
| `APX_NO_ERROR` | Request succeeded |
| `APX_INVALID_ARGUMENT_ERROR` | Field value is invalid |
| `APX_UNSUPPORTED_ERROR` | Opcode or requested option is unsupported |
| `APX_NOT_FOUND_ERROR` | Node, port, subscription, or snapshot was not found |
| `APX_MSG_TOO_LARGE_ERROR` | Result cannot fit and the opcode is not pageable |
| `APX_INVALID_MSG_ERROR` | Header, field, UTF-8, count, or payload size is malformed |
| `APX_INVALID_STATE_ERROR` | Required channel or server state is unavailable |
| `APX_BUSY_ERROR` | Request ID is already active or a resource limit was reached |

If at least the 8-byte request envelope is readable, the server returns a
response with the received `request_id` and `opcode`. If the envelope itself
is truncated, has request ID zero, or exceeds the advertised file size, the
server sends `RMF_CMD_NACK` with the appropriate APX error and closes the
request file. It MUST NOT guess a correlation ID.

An unknown opcode receives `APX_UNSUPPORTED_ERROR`. Reserved flags set in a
version 1 request receive `APX_INVALID_ARGUMENT_ERROR`.

## 7. Requests and responses

### 7.1 Opcode registry

| Value | Name | Purpose |
|:---:|:---|:---|
| `0x0001` | `GET_SERVER_INFO` | Server identity, clocks, and counters |
| `0x0002` | `LIST_CONNECTIONS` | Snapshot-paged connection list |
| `0x0003` | `LIST_NODES` | Snapshot-paged node list |
| `0x0004` | `LIST_ROUTES` | Snapshot-paged provider-to-requirer routes |
| `0x0005` | `LIST_UNCONNECTED_PORTS` | Snapshot-paged dangling ports |
| `0x0006` | `GET_NODE_DEFINITION` | Publish and identify a node's `.apx` file |
| `0x0007` | `SNAPSHOT_NODE_DATA` | Capture and publish raw `.out` and `.in` buffers |
| `0x0010` | `SUBSCRIBE_PORTS` | Subscribe to selected port values |
| `0x0011` | `UNSUBSCRIBE` | Remove one subscription |

Values `0x0000` and `0x8000`-`0xFFFF` are reserved.

### 7.2 `GET_SERVER_INFO`

Request payload: empty.

Success response:

| Field | Type | Description |
|:---|:---:|:---|
| `protocol_minor` | `U16` | Highest compatible version 1 minor, initially `0` |
| `reserved` | `U16` | Zero |
| `server_start_unix_ns` | `U64` | Wall-clock start time |
| `uptime_ns` | `U64` | Monotonic server uptime |
| `connection_count` | `U32` | Active operational and diagnostic connections |
| `node_count` | `U32` | Active APX nodes |
| `route_count` | `U32` | Active provider-to-requirer routes |
| `server_version` | `String16` | Server software version |

The server captures its wall-clock start timestamp and monotonic origin after
successful extension initialization, immediately before the daemon prints
`Server running`. Configuration and extension loading are excluded from uptime.
`server_start_unix_ns` remains fixed for that run; `uptime_ns` is the current
monotonic time minus the saved origin, independent of wall-clock adjustments.
A query before startup completes returns `APX_INVALID_STATE_ERROR`.
Notification timestamps use the same monotonic origin.

Notification timestamps can be correlated to Unix time as:

```text
server_start_unix_ns + timestamp_ns
```

### 7.3 Paged list protocol

The four `LIST_*` commands use a common request:

| Field | Type | Description |
|:---|:---:|:---|
| `snapshot_id` | `U64` | Zero for a new snapshot; otherwise value from prior response |
| `cursor` | `U32` | Zero for first page; otherwise `next_cursor` |
| `max_items` | `U16` | Requested item limit, 1-1024 |
| `reserved` | `U16` | Zero |

The response begins:

| Field | Type | Description |
|:---|:---:|:---|
| `snapshot_id` | `U64` | Non-zero snapshot token |
| `next_cursor` | `U32` | Next cursor, or `0xFFFFFFFF` after the final page |
| `item_count` | `U16` | Items in this response |
| `reserved` | `U16` | Zero |

The server chooses an actual page size that fits the response file's
advertised `FileSize`. All pages for one `snapshot_id` describe one immutable
logical view, even while the live server changes.

Snapshot tokens are scoped to a diagnostic connection. The server may expire
a token after 30 seconds of inactivity or after the final page. An expired,
foreign, or mismatched token returns `APX_NOT_FOUND_ERROR`. A client restarts
from snapshot ID zero.

### 7.4 `LIST_CONNECTIONS`

Each response item is:

| Field | Type | Description |
|:---|:---:|:---|
| `connection_id` | `U32` | Server connection ID |
| `connection_type` | `U8` | APX connection type |
| `connection_state` | `U8` | APX connection state |
| `reserved` | `U16` | Zero |
| `connected_at_ns` | `U64` | Server uptime when accepted |
| `tag` | `String16` | Connection tag |
| `endpoint` | `String16` | Peer address or local socket identity |

### 7.5 `LIST_NODES`

Each response item is:

| Field | Type | Description |
|:---|:---:|:---|
| `node_id` | `U32` | Server diagnostic node ID |
| `connection_id` | `U32` | Owning connection |
| `provide_port_count` | `U32` | Provide ports |
| `require_port_count` | `U32` | Require ports |
| `node_name` | `String16` | APX node name |

Node IDs distinguish nodes by their owning connection and node name, and are
stable only while the node remains attached. Node names need only be unique
within a connection; identically named nodes on different connections MUST
have distinct diagnostic node IDs.

### 7.6 `LIST_ROUTES`

Each item represents one directed provider-to-requirer route:

| Field | Type | Description |
|:---|:---:|:---|
| `provide_node_id` | `U32` | Provider node |
| `provide_port_id` | `U32` | Provider's APX port ID |
| `require_node_id` | `U32` | Requirer node |
| `require_port_id` | `U32` | Requirer's APX port ID |
| `port_name` | `String16` | Matched APX port name |

One provider with three consumers produces three route items. This avoids the
ambiguous `connector_count` field from the earlier draft.

### 7.7 `LIST_UNCONNECTED_PORTS`

Each response item is:

| Field | Type | Description |
|:---|:---:|:---|
| `node_id` | `U32` | Owning node |
| `port_id` | `U32` | APX port ID |
| `direction` | `U8` | `0` = require, `1` = provide |
| `reason` | `U8` | `0` = no provider, `1` = no consumer |
| `reserved` | `U16` | Zero |
| `port_name` | `String16` | APX port name |

### 7.8 `GET_NODE_DEFINITION`

Request:

| Field | Type | Description |
|:---|:---:|:---|
| `node_selector` | `String16` | Node name or `connection_id:node_name` |

A bare name MUST match exactly one operational node across all connections.
Multiple matches return `APX_AMBIGUOUS_NODE_ERROR` (`82`); no match returns
`APX_NOT_FOUND_ERROR`. A qualified selector uses a decimal U32 connection ID
and resolves only within that connection. Both this command and
`SNAPSHOT_NODE_DATA` use these rules.

On success, the server first publishes a temporary fixed RemoteFile named:

```text
apx.diag.v1.def.<request_id>
```

The file contains the exact `.apx` definition bytes. The server then sends this
response payload:

| Field | Type | Description |
|:---|:---:|:---|
| `file_address` | `U32` | Address from the preceding `RMF_CMD_FILE_INFO` |

The client matches the response to the already announced file, sends
`RMF_CMD_FILE_OPEN`, receives the definition using normal RemoteFile transfer,
and then sends `RMF_CMD_FILE_CLOSE`. The server revokes the temporary file
after close, after 30 seconds without an open, or when the diagnostic
connection ends.

This design avoids response chunking, duplicates no file size or digest fields,
and lets RemoteFile enforce normal file bounds.

#### `SNAPSHOT_NODE_DATA` (`0x0007`)

The request is the same `node_selector: String16`, with flags zero. The server
copies both port-data buffers while holding the same node-data lock. Network
transfer occurs after releasing the node-data and topology locks.

For each present direction, publish a temporary fixed RemoteFile:

* `apx.diag.v1.data.<request_id>.p` for provide-port data;
* `apx.diag.v1.data.<request_id>.r` for require-port data.

These are diagnostic-only transport names, not operational `.out`/`.in` files.
The eight-byte response contains `provide_file_address: U32`, then
`require_file_address: U32`. An absent direction uses `RMF_INVALID_ADDRESS`
(`0x7FFFFFFF`). File sizes come from the RemoteFile announcements.

The client opens each announced file, assembles writes by offset until every
advertised byte is received, then closes it. The server revokes closed
temporary files and releases remaining snapshots on diagnostic disconnect.
Captured bytes stay immutable even if the operational data changes.

`apxmon dump` saves the bytes as `<node_name>.out` and/or `<node_name>.in` in the
current directory, overwriting existing files. Connection qualification is
not included in local filenames. There is no application header, terminator,
or encoding conversion in the output files.

### 7.9 `SUBSCRIBE_PORTS`

Request flags:

| Bit | Name | Meaning |
|:---:|:---|:---|
| 0 | `SEND_INITIAL` | Send the current value after successful subscription |
| 1-15 | Reserved | Must be zero |

Request payload:

| Field | Type | Description |
|:---|:---:|:---|
| `selector_count` | `U16` | Number of selectors, 1-256 |
| `reserved` | `U16` | Zero |
| `selectors` | repeated | `node_name: String16`, `port_name: String16` |

Success response:

| Field | Type | Description |
|:---|:---:|:---|
| `subscription_id` | `U32` | Non-zero, connection-scoped identifier |
| `result_count` | `U16` | Equal to `selector_count` |
| `reserved` | `U16` | Zero |
| `results` | repeated | Resolution result below |

Each resolution result is:

| Field | Type | Description |
|:---|:---:|:---|
| `selector_index` | `U16` | Zero-based request selector |
| `reserved` | `U16` | Zero |
| `status` | `U32` | `APX_NO_ERROR` or selector-specific error |
| `node_id` | `U32` | Zero on failure |
| `port_id` | `U32` | Zero on failure |
| `value_size` | `U32` | Serialized APX port size, zero on failure |

The overall response status is success when the request is valid, even if an
individual selector is not found. At least one selector must resolve;
otherwise the overall status is `APX_NOT_FOUND_ERROR` and the payload is empty.

### 7.10 `UNSUBSCRIBE`

Request payload:

| Field | Type | Description |
|:---|:---:|:---|
| `subscription_id` | `U32` | Subscription to remove |

Success response payload: empty.

Unsubscribing an unknown or already removed ID returns
`APX_NOT_FOUND_ERROR`.

## 8. Notifications

### 8.1 Event registry

| Value | Name | Purpose |
|:---:|:---|:---|
| `0x0001` | `PORT_VALUE` | Serialized value of a subscribed port |
| `0x0002` | `SUBSCRIPTION_INVALIDATED` | A selected node or port disappeared |
| `0x00FE` | `NOTIFICATIONS_DROPPED` | Bounded notification queue overflowed |

### 8.2 Notification flags

| Bit | Name | Meaning |
|:---:|:---|:---|
| 0 | `INITIAL_VALUE` | Value was requested by `SEND_INITIAL` |
| 1-15 | Reserved | Sender writes zero; receiver ignores |

### 8.3 `PORT_VALUE`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `node_id` | `U32` | Resolved node |
| `port_id` | `U32` | Resolved port |
| `value` | `Bytes` | Raw APX-serialized port bytes |

The subscription response already supplies `value_size`, so a changed size is
a protocol error. Human-readable decoding uses the node's APX definition and
the normal APX VM; the server does not put JSON or presentation strings on the
wire.

### 8.4 `SUBSCRIPTION_INVALIDATED`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `node_id` | `U32` | Removed node |
| `port_id` | `U32` | Removed port, or `0xFFFFFFFF` for the entire node |
| `reason` | `U32` | `apx_error_t` |

The server removes the affected selector. It removes the subscription after
the last selector is gone.

### 8.5 `NOTIFICATIONS_DROPPED`

Payload:

| Field | Type | Description |
|:---|:---:|:---|
| `first_sequence` | `U64` | First sequence not delivered |
| `dropped_count` | `U64` | Number of consecutive dropped notifications |

The first accepted notification after an overflow window reports that window.
Clients MUST also detect sequence gaps.

## 9. Consistency, concurrency, and backpressure

Query handlers take a state snapshot under the appropriate server lock, then
release the lock before encoding or transmitting a response. They MUST NOT
perform socket writes or invoke registered callbacks while holding an internal
mutex.

Port-change capture uses a bounded non-blocking producer queue. A slow
diagnostic client never blocks signal routing. Overflow drops notifications
and produces `NOTIFICATIONS_DROPPED`; it does not disconnect operational APX
clients.

Servers SHOULD enforce per-connection limits for:

* active request IDs;
* retained list snapshots;
* subscriptions and selected ports;
* response rate; and
* temporary definition files.

Limit exhaustion returns `APX_BUSY_ERROR`.

## 10. `apxmon` command mapping

```text
apxmon [OPTIONS] COMMAND [ARGS...]

Options:
  -c, --connect <ADDR>      Diagnostic endpoint
  --tls                     Use TLS
  --ca-cert <FILE>          Server trust anchor
  --client-cert <FILE>      Diagnostic client certificate
  --client-key <FILE>       Diagnostic client private key
  --json                    Emit JSON or NDJSON
  -h, --help
  -v, --version
```

| CLI command | Protocol operation |
|:---|:---|
| `apxmon info` | `GET_SERVER_INFO` |
| `apxmon clients` | all pages of `LIST_CONNECTIONS` |
| `apxmon nodes` | all pages of `LIST_NODES`, optionally joined with connections |
| `apxmon routing` | all pages of `LIST_ROUTES`, joined with `LIST_NODES` for human-readable endpoint names |
| `apxmon routing --unconnected` | all pages of `LIST_UNCONNECTED_PORTS`, joined with `LIST_NODES` for human-readable node names |
| `apxmon get-apx NODE` | `GET_NODE_DEFINITION`, then open the returned file |
| `apxmon dump NODE` | `SNAPSHOT_NODE_DATA`, then fetch and save each present binary file |
| `apxmon watch NODE:PORT...` | open notify file, then `SUBSCRIBE_PORTS` |
| `apxmon top` | periodic list snapshots plus selected subscriptions |

Human-readable routing output uses one row per directed connection, displaying
the port name, provider endpoint, and requirer endpoint. Each endpoint uses
`connection_id:node_name`, with the same connection ID reported by `apxmon nodes`.
Unconnected ports remain a separate view and use the same qualified node
format. JSON output retains the numeric IDs and existing item
schemas. Node and route lists are separate snapshots; if a referenced node
cannot be resolved, the CLI reports an error and asks the user to retry.

`--json` changes only local output. It does not change the binary diagnostic
protocol.

On `Ctrl+C`, `watch` sends `UNSUBSCRIBE` when possible, closes the notification
file, and closes the RMFP connection cleanly.

## 11. Security requirements

The diagnostic endpoint is privileged. Transport authentication and
authorization happen before the RMFP greeting is accepted.

The server MUST:

* expose these files only on an authorized diagnostic connection;
* avoid leaking endpoint or node information in unauthenticated errors;
* validate every count, length, cursor, ID, and arithmetic operation before
  allocation or access;
* cap allocations by the channel's advertised `FileSize` and configured
  resource limits; and
* revoke temporary definition files when their request lifetime ends.

## 12. Validation requirements

Implementations require tests for:

* NumHeader32 plus AddressHeader parsing with one diagnostic message per
  logical RemoteFile write;
* `MORE_BIT` reassembly before dispatch;
* truncated headers, invalid UTF-8, impossible counts, trailing bytes, unknown
  flags, duplicate request IDs, and oversized messages;
* out-of-order response correlation;
* list pagination while the live topology changes;
* expired and cross-command snapshot tokens;
* node definition publish/open/close/revoke lifecycle;
* subscription setup, initial values, invalidation, queue overflow, and
  sequence gaps;
* disconnect cleanup of requests, snapshots, subscriptions, and temporary
  files; and
* authorization failure before any diagnostic file is announced.
