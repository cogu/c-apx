# Proposed RemoteFile v1.1 Stream File Semantics and Flow Control

## 1. Status and scope

This document proposes additions and clarifications for the RemoteFile v1.1
specification. It is intended to be reviewed and later incorporated into the
authoritative specification. It does not modify the specification itself.

The proposal defines:

* block-preserving semantics for `FileStream`;
* the relationship between NumHeader framing, `MORE_BIT`, logical writes, and
  stream blocks;
* stream-specific open negotiation;
* per-file receive-window flow control;
* stream state, close, revoke, and error behavior; and
* compatibility and validation requirements.

The proposal applies to `FileStream` (`FileType = 5`) in RemoteFile v1.1.
Fixed, dynamic, and device file behavior is unchanged.

## 2. Motivation

RemoteFile v1.1 defines `FileStream` as a streaming FIFO queue, but does not
fully specify:

* whether logical-write boundaries remain visible to the receiver;
* how the maximum block size is agreed;
* how many blocks a receiver can buffer;
* when a sender must stop for a slow receiver;
* how receive credit is returned; or
* how stream-specific errors affect file and connection state.

Normal APX clients have historically used fixed-size files, so these questions
did not affect deployed behavior. The first use of `FileStream` is an
opportunity to define these semantics before compatibility constraints exist.

## 3. Terminology

### 3.1 RMFP packet

An RMFP packet is one NumHeader-framed transport message:

```text
+----------------------+----------------------------------+
| NumHeader16/32       | RemoteFile packet payload        |
| payload byte count   | AddressHeader + fragment bytes   |
+----------------------+----------------------------------+
```

NumHeader frames the packet. It does not by itself define an application
record after multiple stream blocks have been concatenated.

### 3.2 Logical RemoteFile write

A logical write consists of one or more consecutive RMFP packets:

```text
packet 1: MORE_BIT = 1
packet 2: MORE_BIT = 1
packet 3: MORE_BIT = 0
```

The final packet completes the write. The RemoteFile layer reassembles all
fragments before notifying the file consumer.

### 3.3 Stream block

For a `FileStream`, one completed logical RemoteFile write is one stream
block.

A stream block is the unit of:

* atomic delivery;
* block-size enforcement;
* FIFO ordering; and
* receive-window accounting.

RMFP fragments are not stream blocks and do not individually consume window
credit.

### 3.4 Byte-stream adapter

A byte-stream adapter may concatenate stream blocks and expose conventional
`read`, `peek`, and `consume` operations. This adaptation is optional and sits
above the canonical block-preserving `FileStream` interface.

The adapter MUST NOT change the underlying RemoteFile semantics. Applications
that require block boundaries attach directly to the block interface.

## 4. Normative `FileStream` semantics

### 4.1 FIFO of blocks

A `FileStream` is an ordered FIFO sequence of variable-sized blocks.

The following rules are normative:

1. One completed logical RemoteFile write appends exactly one block.
2. The receiver delivers the complete block atomically.
3. Block boundaries are preserved by the RemoteFile file-consumer API.
4. Blocks are delivered in transmission order.
5. A block contains at least one data byte.
6. A block contains no more than the negotiated `BlockSize`.
7. A receiver MUST NOT expose a partial block to the file consumer.
8. A receiver MAY concatenate blocks only through an explicit higher-level
   byte-stream adapter.

An application protocol may assign one application message to each block. In
that case, no additional message-length field is required inside the block.

### 4.2 Addressing

Every new stream block begins at the stream file's published `StartAddress`.

If a block is fragmented:

* the first fragment uses `StartAddress`;
* each continuation fragment uses
  `StartAddress + bytes_already_transmitted_for_this_block`; and
* the final fragment has `MORE_BIT = 0`.

The next block resets its write address to `StartAddress`.

Example:

```text
File StartAddress = 0x20000000

Block A, 6000 bytes:
  packet 1: address=0x20000000, MORE=1, data=4096 bytes
  packet 2: address=0x20001000, MORE=0, data=1904 bytes

Block B, 100 bytes:
  packet 3: address=0x20000000, MORE=0, data=100 bytes
```

The receiver delivers two blocks with sizes 6000 and 100.

### 4.3 Fragment interleaving

Packets belonging to one fragmented logical write MUST be consecutive on the
RMFP connection. No command, file write, or fragment from another logical
write may be interleaved before the packet with `MORE_BIT = 0`.

Writes to other files may occur between complete stream blocks.

This rule permits a receiver to maintain one active reassembly operation per
connection. A future protocol version may relax it by adding explicit transfer
identifiers.

### 4.4 Published `FileSize`

For `FileStream`, the `FileSize` field in `RMF_CMD_FILE_INFO` means:

> The largest stream block that the publisher is capable of transmitting for
> this file.

It is not:

* the total stream length;
* the stream's storage capacity;
* the receiver's negotiated block size; or
* a requirement to preallocate a buffer of that size.

The stream has no protocol-defined total length.

## 5. Stream-open negotiation

### 5.1 Rationale

`RMF_CMD_FILE_OPEN` contains only a file address. It cannot communicate:

* the largest block accepted by the receiver; or
* the number of unconsumed blocks the receiver can buffer.

Stream files therefore use a stream-specific open exchange. Existing
`RMF_CMD_FILE_OPEN` remains unchanged for non-stream files.

### 5.2 Proposed command identifiers

The following command values are proposed:

| Command | Proposed value | Direction |
|:---|:---:|:---|
| `RMF_CMD_STREAM_OPEN` | `25` | File receiver to publisher |
| `RMF_CMD_STREAM_OPEN_RSP` | `26` | File publisher to receiver |
| `RMF_CMD_STREAM_WINDOW_UPDATE` | `27` | File receiver to publisher |
| `RMF_CMD_STREAM_ABORT` | `28` | Either peer |

Following `RMF_CMD_PUBLISH_SIGNED_FILE` (`23`) and `RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE` (`24`), stream file operations are assigned contiguous opcodes `25` through `28`. Symbolic names and layouts are normative for this proposal.

### 5.3 `RMF_CMD_STREAM_OPEN`

The receiver sends this command after a `FileStream` has been announced.

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `CmdType` | `U32LE` | `RMF_CMD_STREAM_OPEN` |
| 4 | `StartAddress` | `U32LE` | Published stream address |
| 8 | `MaxBlockSize` | `U32LE` | Largest block accepted by the receiver |
| 12 | `WindowBlocks` | `U16LE` | Maximum unconsumed complete blocks |
| 14 | `Flags` | `U16LE` | Zero in this proposal |

Total size: **16 bytes**.

Constraints:

* `MaxBlockSize` MUST be greater than zero.
* `WindowBlocks` MUST be greater than zero.
* `Flags` MUST be zero.
* `StartAddress` MUST identify a published `FileStream`.
* The stream MUST NOT already be open.

The requested maximum block size may be greater or smaller than the published
`FileSize`. The publisher selects the smaller supported value in its response.

### 5.4 `RMF_CMD_STREAM_OPEN_RSP`

The publisher sends exactly one response to every syntactically valid
`RMF_CMD_STREAM_OPEN`.

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `CmdType` | `U32LE` | `RMF_CMD_STREAM_OPEN_RSP` |
| 4 | `StartAddress` | `U32LE` | Address from the request |
| 8 | `Status` | `U32LE` | `apx_error_t`; zero is success |
| 12 | `BlockSize` | `U32LE` | Negotiated maximum block size |
| 16 | `WindowBlocks` | `U16LE` | Accepted fixed receive window |
| 18 | `Flags` | `U16LE` | Zero in this proposal |

Total size: **20 bytes**.

On success:

```text
BlockSize <= published FileSize
BlockSize <= requested MaxBlockSize
WindowBlocks <= requested WindowBlocks
BlockSize > 0
WindowBlocks > 0
```

The publisher MAY select smaller values because of local resource limits.

On failure:

* `Status` is non-zero;
* `BlockSize` is zero;
* `WindowBlocks` is zero;
* `Flags` is zero; and
* the stream remains closed.

The explicit response correlates the result by `StartAddress` and communicates
effective values before any stream data is sent.

### 5.5 Opening sequence

```text
Receiver                                           Publisher
   |                                                   |
   |<-- RMF_CMD_FILE_INFO(FileStream, FileSize) -------|
   |                                                   |
   |--- RMF_CMD_STREAM_OPEN --------------------------->|
   |    MaxBlockSize, WindowBlocks                      |
   |                                                   |
   |<-- RMF_CMD_STREAM_OPEN_RSP ------------------------|
   |    Status=0, BlockSize, WindowBlocks               |
   |                                                   |
   |<== stream blocks, subject to receive window =======|
```

The publisher MUST NOT transmit a stream block before sending a successful
open response.

## 6. Receive-window flow control

### 6.1 Purpose

TCP and similar transports provide connection-level byte backpressure.
`FileStream` additionally needs per-file application-consumption flow control
to:

* bound receiver queue memory;
* isolate multiple stream files on one connection;
* prevent a slow stream consumer from consuming an unbounded send queue; and
* distinguish network receipt from application consumption.

### 6.2 Counters

Each opened stream maintains two unsigned 64-bit counters:

| Counter | Owner | Initial value | Meaning |
|:---|:---|:---:|:---|
| `TransmittedBlockCount` | Publisher | `0` | Complete blocks transmitted |
| `ConsumedBlockCount` | Receiver | `0` | Complete blocks consumed or released |

The number of outstanding blocks is:

```text
OutstandingBlocks = TransmittedBlockCount - ConsumedBlockCount
```

The publisher may begin another block only when:

```text
OutstandingBlocks < WindowBlocks
```

The publisher increments `TransmittedBlockCount` once for each completed
logical write sent. RMFP fragments do not increment it.

### 6.3 Consumption

A block is consumed when the receiving application:

* finishes processing it; or
* explicitly takes ownership or copies it into storage outside the negotiated
  stream receive queue.

Receipt from the socket alone is not consumption.

An implementation MUST NOT return credit while the block still occupies a
slot in the bounded receive queue.

### 6.4 `RMF_CMD_STREAM_WINDOW_UPDATE`

The receiver sends a cumulative consumption update:

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `CmdType` | `U32LE` | `RMF_CMD_STREAM_WINDOW_UPDATE` |
| 4 | `StartAddress` | `U32LE` | Open stream address |
| 8 | `ConsumedBlockCount` | `U64LE` | Cumulative consumed blocks |

Total size: **16 bytes**.

The value:

* MUST be greater than or equal to the previous accepted value;
* MUST be less than or equal to `TransmittedBlockCount`; and
* MUST refer to an open stream.

An update equal to the previous value is a harmless duplicate and has no
effect. A lower value or a value greater than `TransmittedBlockCount` is a
protocol error.

A cumulative counter is used instead of an additive credit because it is
idempotent and simple to validate.

### 6.5 Update batching

The receiver need not send one update for every consumed block. It may batch
updates.

The receiver SHOULD send an update when either:

* at least half of the negotiated window has been consumed since the previous
  update; or
* no unconsumed block remains and new data is expected.

For `WindowBlocks = 1`, every consumed block necessarily produces an update
before another block can be transmitted.

### 6.6 Control-message progress

Stream window exhaustion stops data blocks for that stream only. It MUST NOT
prevent either peer from processing Control Area commands, including window
updates and file close commands.

An implementation SHOULD avoid placing more stream data into its transport
send queue than permitted by current stream credit.

### 6.7 Counter exhaustion

If either block counter reaches `UINT64_MAX`, the publisher MUST stop the
stream and the peers MUST close and reopen it to reset the counters. Counter
wraparound is forbidden.

## 7. Block transmission rules

After negotiation:

1. Every block size is in the range `1..BlockSize`.
2. The first fragment starts at the stream `StartAddress`.
3. Continuation addresses are contiguous.
4. No fragment chain is interleaved with another write.
5. Starting a block consumes one available window position.
6. The receiver delivers a block only after its final fragment.
7. The receiver returns credit only after application consumption.

The sender may use blocks smaller than `BlockSize`. The negotiated size is a
maximum, not a preferred fixed size.

## 8. Close and revoke behavior

### 8.1 Receiver close

The receiver closes an open stream using the existing
`RMF_CMD_FILE_CLOSE`.

After receiving the close:

* the publisher sends no new stream blocks;
* unsent queued blocks are discarded or returned to their application owner;
* in-progress transmission behavior follows Section 8.4;
* stream counters and negotiated values are discarded; and
* reopening starts new counters at zero.

### 8.2 Publisher revoke

`RMF_CMD_REVOKE_FILE` immediately closes the stream.

The receiver:

* stops accepting blocks for the address;
* discards any incomplete fragment chain;
* releases queued blocks;
* discards counters and negotiated values; and
* reports revocation to the file consumer.

### 8.3 Connection loss

Connection loss implicitly closes all streams. Negotiated parameters and
counters do not survive reconnection.

The application protocol is responsible for any resume or replay behavior.

### 8.4 Close during a fragmented block

Control commands cannot be interleaved inside a fragmented logical write in
the same transmission direction. A close can arrive in the reverse direction
while the publisher is transmitting a fragmented block.

If transmission of a block has begun when the publisher receives a close, the
publisher completes that block and then closes the stream. The receiver accepts
that final complete block. The publisher MUST NOT begin another block.

If the physical connection ends during a fragmented block, the receiver
discards the incomplete block and does not deliver it.

### 8.5 Stream abort

Either peer may abort one stream without closing the RMFP connection:

| Offset | Field | Type | Description |
|:---:|:---|:---:|:---|
| 0 | `CmdType` | `U32LE` | `RMF_CMD_STREAM_ABORT` |
| 4 | `StartAddress` | `U32LE` | Open stream address |
| 8 | `Status` | `U32LE` | Non-zero `apx_error_t` describing the failure |

Total size: **12 bytes**.

After sending or receiving an abort:

* no new block is started;
* any incomplete block is discarded and not delivered;
* queued blocks are released;
* counters and negotiated values are discarded; and
* the published file returns to the announced/discovered state.

The file may be opened again with a new stream-open exchange. An abort does not
revoke the file.

## 9. State machines

### 9.1 Publisher state

```text
UNPUBLISHED
    |
    | RMF_CMD_FILE_INFO(FileStream)
    v
ANNOUNCED
    |
    | valid RMF_CMD_STREAM_OPEN
    v
OPENING
    |
    | successful RMF_CMD_STREAM_OPEN_RSP
    v
OPEN
    |
    +-- RMF_CMD_FILE_CLOSE --> ANNOUNCED
    |
    +-- RMF_CMD_STREAM_ABORT --> ANNOUNCED
    |
    +-- RMF_CMD_REVOKE_FILE --> UNPUBLISHED
    |
    +-- connection loss --> UNPUBLISHED
```

A failed open response returns to `ANNOUNCED`.

### 9.2 Receiver state

```text
UNKNOWN
    |
    | receive RMF_CMD_FILE_INFO(FileStream)
    v
DISCOVERED
    |
    | send RMF_CMD_STREAM_OPEN
    v
OPEN_PENDING
    |
    +-- success response --> OPEN
    |
    +-- failure response --> DISCOVERED

OPEN
    |
    +-- send RMF_CMD_FILE_CLOSE --> DISCOVERED
    |
    +-- send/receive RMF_CMD_STREAM_ABORT --> DISCOVERED
    |
    +-- receive RMF_CMD_REVOKE_FILE --> UNKNOWN
    |
    +-- connection loss --> UNKNOWN
```

Stream blocks are valid only in `OPEN`.

## 10. Errors

### 10.1 Open-response status

Suggested status values include:

| Condition | Status |
|:---|:---|
| Address is unknown | `APX_FILE_NOT_FOUND_ERROR` |
| File is not `FileStream` | `APX_INVALID_FILE_ERROR` |
| Zero block size, zero window, or non-zero flags | `APX_INVALID_ARGUMENT_ERROR` |
| Stream is already open or opening | `APX_BUSY_ERROR` |
| Requested feature is unsupported | `APX_UNSUPPORTED_ERROR` |
| Local resource limit prevents opening | `APX_BUSY_ERROR` |

### 10.2 Data-path protocol errors

The following are protocol errors:

* a block arrives before a successful open response;
* a block exceeds negotiated `BlockSize`;
* the publisher exceeds `WindowBlocks`;
* a continuation address is not contiguous;
* fragments are interleaved;
* a zero-length stream block is sent;
* a window update references a closed stream;
* `ConsumedBlockCount` decreases; or
* `ConsumedBlockCount` exceeds transmitted blocks.

On a stream-specific error, the peer:

1. sends `RMF_CMD_STREAM_ABORT` with the affected address and error status;
2. closes local stream state for that file; and
3. ignores further data writes to that stream until it is reopened.

An error that makes packet boundaries or fragment ownership ambiguous is a
connection-level protocol error and the peer SHOULD close the RMFP connection.

`RMF_CMD_NACK` remains appropriate for malformed control commands that cannot
be associated with a known stream address.

No malformed or oversized block is delivered partially to the file consumer.

## 11. Compatibility

### 11.1 Existing file types

Existing behavior remains unchanged:

| File type | Open command |
|:---|:---|
| `FixedFile` | `RMF_CMD_FILE_OPEN` |
| `DynamicFile8/16/32` | `RMF_CMD_FILE_OPEN` |
| `DeviceFile` | `RMF_CMD_FILE_OPEN` |
| `FileStream` | `RMF_CMD_STREAM_OPEN` |

### 11.2 Ordinary open against a stream

An RMFP v1.1 peer implementing this proposal MUST reject
`RMF_CMD_FILE_OPEN` for a `FileStream` with `APX_UNSUPPORTED_ERROR`.

This prevents an unnegotiated stream mode from becoming a second behavior that
must be supported indefinitely.

### 11.3 Stream open against another file type

`RMF_CMD_STREAM_OPEN` for a non-stream file receives
`RMF_CMD_STREAM_OPEN_RSP` with `APX_INVALID_FILE_ERROR`.

### 11.4 Peers without the extension

An older peer will treat `RMF_CMD_STREAM_OPEN` as unsupported and respond with
`RMF_CMD_NACK`, or otherwise reject the command according to RemoteFile v1.1
unknown-command behavior.

Applications requiring `FileStream` treat this as capability absence. They do
not fall back to unnegotiated ordinary file open.

No new greeting attribute is required for initial deployment because file
publication and the open response already provide capability detection.

## 12. Resource and security requirements

An implementation MUST configure limits for:

* maximum published stream `FileSize`;
* maximum negotiated `BlockSize`;
* maximum `WindowBlocks`;
* maximum aggregate receive memory per connection;
* maximum aggregate send memory per connection; and
* maximum number of simultaneously open streams.

Published `FileSize` and requested values are untrusted inputs. All
multiplication and addition used for memory budgets MUST be checked for
overflow.

The implementation SHOULD allocate reassembly storage on demand. Publishing or
opening a stream MUST NOT automatically allocate:

```text
BlockSize * WindowBlocks
```

unless that allocation is explicitly part of the implementation's bounded
queue design.

Authentication and authorization are transport/application concerns. Opening
a stream does not grant permissions beyond those already assigned to the RMFP
connection.

## 13. Interaction with application protocols

### 13.1 Message-oriented protocols

Applications such as APX diagnostics and event logging may define:

```text
one application message = one FileStream block
```

The RemoteFile layer then supplies the message boundary. The application
payload does not repeat a record or message length.

### 13.2 Payloads larger than `BlockSize`

An application object larger than `BlockSize` must use one of these strategies:

* transfer the object as a fixed or dynamic RemoteFile;
* split it into independently meaningful stream blocks;
* use application-defined segmentation identifiers; or
* reject it as too large.

The RemoteFile stream layer does not concatenate multiple complete blocks back
into one atomic application message.

### 13.3 Lossless and lossy producers

RemoteFile window flow control is lossless: it stops the publisher when credit
is exhausted.

An application may place a bounded producer queue before the RemoteFile stream:

```text
application event source
        |
        v
bounded producer queue
        |
        v
RemoteFile window-controlled stream
```

A logging application may drop new producer events when its own queue is full
and report the loss in a later block. A diagnostic request/response protocol
would normally retain backpressure and avoid dropping.

Loss policy is therefore an application decision, not an implicit
`FileStream` behavior.

## 14. Worked example

The publisher announces:

```text
FileName: server.log
FileType: FileStream
FileSize: 262144
StartAddress: 0x20000000
```

The receiver requests:

```text
MaxBlockSize: 65536
WindowBlocks: 4
```

The publisher accepts:

```text
BlockSize: 65536
WindowBlocks: 4
```

The publisher sends four blocks:

```text
TransmittedBlockCount = 4
ConsumedBlockCount = 0
OutstandingBlocks = 4
```

The window is exhausted, so the publisher stops sending this stream.

The receiver consumes three blocks and sends:

```text
RMF_CMD_STREAM_WINDOW_UPDATE
StartAddress = 0x20000000
ConsumedBlockCount = 3
```

The publisher now has:

```text
OutstandingBlocks = 4 - 3 = 1
Available window positions = 4 - 1 = 3
```

It may transmit three more blocks.

The maximum payload retained in the negotiated receive queue is approximately:

```text
65536 * 4 = 262144 bytes
```

plus reassembly and queue metadata.

## 15. Optional future extensions

The initial proposal deliberately omits:

* minimum inter-block delay;
* dynamic window resizing;
* priority between streams;
* resumable streams;
* per-block checksums;
* per-block acknowledgements beyond cumulative consumption;
* multiplexed fragment transfer identifiers; and
* half-close semantics.

Potential future mappings inspired by ISO 15765-2 include:

| Concept | Possible extension |
|:---|:---|
| Separation time | Minimum interval between block starts |
| Flow-control wait | Explicit temporary zero-credit state |
| Overflow | `RMF_CMD_STREAM_ABORT` with a resource error |
| Dynamic block size | Window update carrying new capacity |

Reserved `Flags` fields permit compatible additions when their behavior is
fully specified.

## 16. Required tests

Implementations require tests for:

### Block semantics

* one packet forming one block;
* multiple `MORE_BIT` fragments forming one block;
* the next block resetting to `StartAddress`;
* preservation of block boundaries;
* rejection of zero-length and oversized blocks;
* rejection of non-contiguous and interleaved fragments; and
* no partial delivery after connection loss.

### Open negotiation

* receiver maximum smaller than published `FileSize`;
* published `FileSize` smaller than receiver maximum;
* publisher selecting smaller local limits;
* zero size, zero window, unknown flags, unknown address, wrong file type, and
  duplicate open;
* no block before successful response; and
* ordinary `RMF_CMD_FILE_OPEN` rejected for streams.

### Window behavior

* `WindowBlocks = 1`;
* publisher stopping exactly at the negotiated window;
* batched cumulative updates;
* harmless duplicate update;
* decreasing and excessive consumed counts;
* per-file isolation with multiple streams;
* control-message progress while a stream window is exhausted; and
* counter reset after close and reopen.

### Lifecycle and resource limits

* receiver close with queued blocks;
* stream abort in both directions;
* publisher revoke;
* connection loss during reassembly;
* aggregate per-connection memory limits;
* integer-overflow protection; and
* cleanup of every queue and reassembly buffer.

## 17. Suggested integration into RemoteFile v1.1

When this proposal is accepted, the authoritative RemoteFile v1.1
specification should be updated in these places:

1. Add block-preserving `FileStream` semantics to the file-type section.
2. Clarify that stream `FileSize` is the publisher's maximum block size.
3. Add the four stream command identifiers to the command registry.
4. Add command layouts and the open sequence.
5. Add receive-window counter rules.
6. Add stream lifecycle and error behavior.
7. Update the v1.0/v1.1 comparison table.
8. Add conformance examples and malformed-input requirements.

Implementations should add stream support only after these semantics and final
command values are agreed, so the first deployed behavior is the behavior that
remains compatible.
