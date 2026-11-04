# Design Proposal: Compressed APX Definition Transfer

* **Status**: Proposed; no compressed-transfer implementation is implied.
* **Backlog**: [Item 16](../Backlog.md#16-compressed-transfer-of-apx-node-definitions-signed-and-unsigned).
* **Depends on**: [Item 20: Compression Backend and CMake Integration](../Backlog.md#20-compression-backend-and-cmake-integration).
* **Affected Components**: RemoteFile publication and reception, server node loading, dynamic clients, and future py-apx/APX-ES integration.
* **Protocol**: [RemoteFile v1.1 compressed definition transfer](../../apx/specifications/protocols/remotefile1_1.md#compressed-definition-transfer).

## 1. Motivation and Scope

An embedded APX node can have all pack/unpack functions generated as C code while still publishing its full APX definition on every connection. Sending that text through a bandwidth-constrained link, a gateway, and TCP adds startup latency even though the embedded node never needs to parse it.

The generator should optionally compress the definition once, on the development machine, and emit the resulting bytes into the firmware's read-only data. The embedded runtime only publishes metadata and serves those bytes. Compression must happen before the constrained link to save bandwidth on that link; compression added only at the gateway is insufficient.

The same transfer format should serve dynamic clients, including future implementations in other languages. Compression and signing are independent: both unsigned and signed definitions can be compressed. Existing uncompressed transfer remains supported.

This proposal does not implement the APX-ES runtime rewrite, modify py-apx, compress signal files, or implement the cache from [item 14](../Backlog.md#14-node-definition-cache-text--binary-form).

## 2. Decisions

| Topic | Decision |
|:---|:---|
| Algorithm and wrapper | One zlib stream (RFC 1950) containing DEFLATE (RFC 1951), without a preset dictionary |
| Authentication | Optional at the feature level; separate unsigned and signed publication commands preserve existing policy |
| Generated representation | Explicit generation-time choice: uncompressed text or compressed binary data |
| Legacy compatibility | Uncompressed unsigned generation retains RMFP 1.0 file publication |
| Runtime fallback | Not required; no probing, timeout-based downgrade, or dual representation in generated firmware |
| C codec backend | Build-time zlib/miniz selection provided by item 20; the protocol is backend-independent |
| Cache dependency | None; optional integration uses the uncompressed definition |

"Uncompressed" describes plain APX text. It must not be confused with *raw DEFLATE*, which is a compressed stream without the zlib wrapper.

## 3. Wire Format and Interoperability

Use `CompressionType = 1`, named `ZlibDeflate`, for the zlib-wrapped representation. Require DEFLATE compression method 8, a window of at most 32 KiB, and no preset dictionary. Reject raw DEFLATE, gzip, concatenated streams, and trailing data rather than guessing the encoding.

The zlib wrapper adds six bytes per definition compared with raw DEFLATE when no dictionary is used. It provides explicit format information and an Adler-32 checksum. That checksum detects accidental corruption; it is not cryptographic authentication.

Available APIs include:

| Client implementation | zlib-format support |
|:---|:---|
| Python | Standard-library `zlib` module, where built with zlib support |
| Node.js | Built-in `node:zlib` |
| C# | `System.IO.Compression.ZLibStream` (.NET 6+) |
| Java | `java.util.zip.Deflater` / `Inflater`, with the normal zlib wrapper |
| Go | Standard-library `compress/zlib` |
| Rust | A crate such as `flate2`; no DEFLATE codec is provided by `std` |
| C | An implementation library such as zlib or miniz; no ISO C library codec exists |

Implementations may use different libraries and compression levels. They must recover identical definition bytes; compressed outputs need not be bit-identical.

### 3.1 Publication Commands

The agreed allocation preserves all RMFP 1.0 values and groups the unreleased RMFP 1.1 commands:

| ID | Command | Purpose |
|:---:|:---|:---|
| 3 | `RMF_CMD_FILE_INFO` / `PUBLISH_FILE` | Existing unsigned, uncompressed publication; unchanged |
| 20 | `RMF_CMD_ACCEPT_HEADER` | Header acceptance; unchanged |
| 21 | `RMF_CMD_PUBLISH_SIGNED_FILE` | Signed, uncompressed publication |
| 22 | `RMF_CMD_PUBLISH_COMPRESSED_FILE` | Unsigned, compressed publication |
| 23 | `RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE` | Signed, compressed publication |
| 24 | `RMF_CMD_STREAM_OPEN` | Open stream |
| 25 | `RMF_CMD_STREAM_OPEN_RSP` | Stream open response |
| 26 | `RMF_CMD_STREAM_WINDOW_UPDATE` | Stream flow control |
| 27 | `RMF_CMD_STREAM_ABORT` | Abort stream |

The C publication and stream constants use the existing `_MSG` suffix. Compressed unsigned publication has a 56-byte fixed header with an optional SHA-256 digest; compressed signed publication has an 88-byte fixed header with an ECDSA-P256 signature. Both carry the compressed size, uncompressed size, compression type, and original `.apx` name. Detailed byte offsets and validation rules belong to the protocol specification, not native C struct layouts.

Implementation work includes cleaning up [remotefile.h](../apx_core/include/apx/remotefile.h), [remotefile.c](../apx_core/src/remotefile.c), dispatch, and [wire-format tests](../apx_core/test/testsuite_remotefile.c). Remove obsolete `RMF_CMD_CONNECTION_CREATE`, `RMF_CMD_CONNECTION_REVOKE`, `RMF_CMD_PUBLISH_REMOTE_FILE_MSG`, and `RMF_CMD_PUBLISH_SIGNED_REMOTE_FILE_MSG` constants and any unused encoders/decoders, declarations, or tests belonging to them. Renumber signed publication and all four stream commands together, and add both compressed-publication commands. Earlier RMFP 1.1 assignments are superseded, not retained as aliases; do not change any RMFP 1.0 opcode.

Monitoring and logging events use an independent event namespace, as exemplified by [apx_log_event.h](../app/apx_log/include/apx_log_event.h). Their event IDs remain unchanged and do not reserve RemoteFile command IDs.

### 3.2 Transport and Logical Definition

The published RemoteFile address range spans `CompressedSize` bytes. Data-write offsets, file bounds, and transfer completion refer to those encoded bytes, not to `UncompressedSize`. The receiver must validate that the entire encoded range fits below the control area without integer overflow.

When transfer is needed, use normal `FILE_OPEN` and data-write fragmentation; an unsigned cache hit can avoid the open entirely. Do not introduce a new transport framing protocol or a new file extension. A compressed definition is immutable while published. Each open transfers the entire encoded stream from offset zero, with sequential, non-overlapping writes and the final `MORE_BIT` cleared only at the declared end. Partial decompression state must not survive reopening or reconnecting.

After decoding, the logical definition contains `UncompressedSize` bytes of APX text. Keep its ownership and size separate from the encoded file's address map. Parsing, hashing, caches, and definition inspection operate on logical text, never on a compressed byte buffer mistaken for text. Any implementation needing a local C-string terminator allocates it separately; it is not part of either advertised size.

The gateway may change framing between transport links, but it must preserve the publication metadata and encoded payload. Supporting that behavior in a particular gateway is an integration requirement, not an assumed property of existing gateway software.

## 4. Code Generation and Compatibility

### 4.1 Generation Contract

py-apx should expose an explicit generation-time choice, defaulting to the existing uncompressed representation for compatibility:

* **Uncompressed**: Emit the existing definition text and length. Unsigned legacy mode uses `RMF_CMD_PUBLISH_FILE_MSG` (the RMFP 1.0 `FILE_INFO` command, ID 3) and an RMFP 1.0-compatible connection path.
* **Compressed**: Emit a `const` binary byte array, encoded length, original length, and encoding metadata. Use the applicable compressed-publication command and a compression-capable server.

The user selects the representation based on the deployed server's supported features. An RMFP 1.1 version string alone does not distinguish older development builds from a compression-capable implementation; document the first supporting server release when implemented.

Generated code must use explicit byte counts, never `strlen()` on encoded data. The runtime needs no compressor or inflater and need not retain the original text. Existing generated serialization functions and signal layouts remain unchanged.

Signing and digest generation operate on the exact definition bytes the generated node would otherwise publish, after any generator transformations. Those bytes may differ from the input `.apx` reference file. Exclude the implicit C-string terminator; do not normalize line endings, reorder declarations, or otherwise transform the text after signing or during reception.

Signing may happen during generation or a build step, just like compression. A signed+compressed APX-ES node can store only the compressed array, lengths, and signature; it needs no runtime codec or on-device signing key to serve the definition.

### 4.2 Compatibility Matrix

| Generated representation | Publication | v0.2.8 server | Compression-capable server |
|:---|:---|:---:|:---:|
| Uncompressed, unsigned | RMFP 1.0 `PUBLISH_FILE` / `FILE_INFO` | Supported | Supported, subject to unsigned-node policy |
| Uncompressed, signed | RMFP 1.1 `PUBLISH_SIGNED_FILE` | Not supported | Supported |
| Compressed, unsigned | RMFP 1.1 `PUBLISH_COMPRESSED_FILE` | Not supported | Supported, subject to unsigned-node policy |
| Compressed, signed | RMFP 1.1 `PUBLISH_SIGNED_COMPRESSED_FILE` | Not supported | Supported |

A compressed-only generated node connected to an incompatible server is a deployment mismatch, not a request for automatic fallback. The application must report the connection failure; legacy servers are not assumed to produce a structured NACK for an unknown command. No compressed bytes may be sent under an uncompressed announcement.

Do not silently remove signing to reach an older server. Unsigned legacy generation is a deliberate user/deployment choice, not a cryptographic downgrade path.

### 4.3 Dynamic Clients

Dynamic clients may compress definition bytes once before publication using their native runtime APIs. They may retain uncompressed publication when it is preferable, including small definitions for which the complete transfer would not become smaller. Compressor level, selection API, and client defaults are implementation decisions, not protocol requirements. Runtime capability negotiation is outside this item.

## 5. Receiver Pipeline, Integrity, and Limits

1. Validate command length, file name, fixed-file type, compression identifier, integrity metadata, sizes, address range, and local resource limits before accepting the publication or allocating buffers.
2. Reject unsigned publication immediately when signed nodes are required. No decompression is needed to make that policy decision.
3. If unsigned publication includes SHA-256 and caching is available, look for a valid matching local text or compiled definition as described below. A usable hit supplies the logical definition without opening the remote file, allocating transfer buffers, or creating decoder state.
4. Otherwise, open the accepted file and receive exactly `CompressedSize` bytes using the ordinary transfer-completion rules. Signed definitions always follow this path.
5. Decode with a strict output bound. Require a valid zlib header, complete stream termination, valid Adler-32, consumption of all encoded bytes, and exactly `UncompressedSize` output bytes.
6. For unsigned transferred content with a digest, check SHA-256 over the decoded bytes. For signed content, verify the existing ECDSA-P256/SHA-256 signature over those bytes against configured trusted keys.
7. After validation, use the uncompressed definition's SHA-256 to look for compatible compiled content, computing it if not already available. On a hit, skip parsing/compilation; otherwise parse and compile normally. Only a validated transfer or usable unsigned cache hit can make the node available.

The implementation must define maximum compressed and uncompressed bytes per definition and an aggregate budget for in-flight transfers. Admission control applies to both compressed input and decoded output; advertised sizes are not permission to allocate arbitrary memory. Size calculations, including any extra terminator, must be overflow-safe. Concrete limits and their configuration mechanism remain implementation decisions.

Do not mandate buffering the entire input plus output: bounded incremental decoding or bounded whole-buffer decoding is acceptable. Choose after measuring peak memory, complexity, and CPU cost. Either approach must release buffers, decoder state, and reservations on rejection, allocation failure, timeout, revoke, reopen, or disconnect. Keep decoded data untrusted until applicable integrity checks succeed.

Report unsupported features, malformed streams, length/digest mismatches, and signature failures explicitly using existing APX errors, NACK where the connection is usable, and normal diagnostics. Never retry malformed or unauthenticated content as an alternative encoding. Follow existing concurrency rules, including invoking user callbacks only after releasing internal locks.

Signatures authenticate definition content, not client identity, freshness, or possession of a signing key. Compression adds no new authentication guarantees.

## 6. Cache and Inspection Integration

Compression works with caching disabled or absent. If item 14 is implemented:

* SHA-256 cache keys refer to the uncompressed definition, independent of codec library, compression level, or transfer representation.
* Either unsigned publication (`PUBLISH_FILE` or `PUBLISH_COMPRESSED_FILE`) may use an advertised SHA-256 to resolve a valid local text or compiled entry before `FILE_OPEN`. Check the advertised node identity and logical definition size, cached-content integrity, and compiled-format/compiler/VM compatibility as applicable. A hit skips definition transfer and, for the compressed form, decompression; a text-only hit still requires compilation.
* If caching is disabled, the digest is absent, or no usable matching entry exists, download normally. A compiled lookup is still possible after validating and hashing received text, even when no digest was advertised.
* An unsigned digest is not proof of authenticity and cannot satisfy a signed-node requirement. Apply signed-node policy before any unsigned cache lookup.
* For either signed publication (`PUBLISH_SIGNED_FILE` or `PUBLISH_SIGNED_COMPRESSED_FILE`), always request the definition and verify the received uncompressed bytes before reusing compiled content. Bounded decompression and stream validation precede signature verification for compressed transfers. Under this policy, no cache hit bypasses signed transfer or verification.
* Cache failures retain the established parsing/compilation path for validated text. Compression errors do not become cache fallback successes.

Review receiver, file-manager, node-loading, monitoring, and definition-inspection surfaces together. Preserve encoded transfer sizes where they describe RemoteFile address space, and distinguish decoded sizes where they describe the logical definition. Do not expand observer wire formats or add compression statistics unless needed to preserve existing behavior.

## 7. Compression Backend Dependency (Item 20)

[Item 20](../Backlog.md#20-compression-backend-and-cmake-integration) owns the internal C codec API, backend implementation, dependency acquisition, CMake configuration, and codec-level tests. It supports both compression for dynamic C clients and bounded decompression for receivers, selecting exactly one backend at build time.

The agreed defaults are zlib on Linux and miniz on Windows, with explicit CMake overrides. zlib uses package discovery; miniz may use an installed package or a pinned FetchContent archive, with offline/local-source support. No new Git submodule, mandatory vcpkg setup, runtime backend selector, or general-purpose compression framework is required. Detailed build requirements and acceptance criteria belong to item 20.

Item 16 consumes that API without including vendor headers or scattering backend conditionals through the RemoteFile code. Dynamic clients retain their original logical definition and prepare a separate compressed transfer representation once before publication, rather than compressing individual fragments. Generated APX-ES nodes that only serve precompressed data need neither runtime library; py-apx and other-language clients use their own compatible codecs.

Both C libraries have permissive licenses. Authoritative licenses: [zlib](https://www.zlib.net/zlib_license.html) and [miniz](https://github.com/richgel999/miniz/blob/master/LICENSE).

## 8. Performance Evidence and Goals

A local experiment on three existing generated definition strings, using zlib level 9 and verifying exact round trips, produced:

| Payload | Original bytes | zlib bytes | Payload reduction |
|:---|---:|---:|---:|
| Small definition | 5,834 | 622 | 89.34% |
| Medium definition | 21,999 | 4,146 | 81.15% |
| Large definition | 63,410 | 12,693 | 79.98% |
| **Combined** | **91,243** | **17,461** | **80.86%** |

These are illustrative payload measurements, not public interoperability fixtures, universal ratio guarantees, or startup benchmarks. In particular, the largest sample does not exceed an 80% reduction. The original source definitions need not have the same byte layout as the generated strings.

Benchmark definition payload bytes, publication/framing overhead, constrained-link transfer time, decompression CPU/peak memory, and end-to-end node readiness separately. Compare against identical uncompressed definitions and server settings, with cache disabled for transfer measurements. Report cache-enabled startup separately if available. Do not retain the old unmeasured "~0.3 ms for 140 KB" claim or require a universal ">80% latency" improvement.

## 9. Acceptance Tests and Delivery Boundaries

* Encode/decode both compressed command layouts, integer byte order, allowed field values, maximum file names, and address/size boundaries. Assert header acceptance at 20, publication at 3 and 21-23, and stream control at 24-27, with unique IDs. Preserve every RMFP 1.0 assignment and remove obsolete command helpers/tests without changing independent application event IDs.
* Exercise all four signed/unsigned and compressed/uncompressed combinations, with signed-node policy enabled and disabled; preserve existing uncompressed behavior.
* Verify generated binary-array lengths, embedded zero bytes, exact reconstructed definition bytes, and unchanged generated signal serialization. Test the unsigned uncompressed generation path against v0.2.8, including its legacy greeting and publication.
* Publish valid compressed definitions with caching disabled. Check node/port metadata, initial values, and routed signals against the equivalent uncompressed case.
* Run transfer integration tests with each backend supplied by item 20; codec/build infrastructure tests remain owned by item 20.
* Add small public test definitions and interoperability vectors. Verify Python-produced zlib streams using the selected C decoder and C-produced streams using Python; include another runtime such as Node.js where available. Do not require byte-identical compressor output.
* Fragment payloads at arbitrary transport boundaries, including inside zlib headers/trailers. Test reopen, revoke, timeout, and disconnect during reception/decompression, with no leaks or stale state.
* Reject unknown encodings, raw DEFLATE/gzip mislabeled as zlib, preset dictionaries, corrupt checksums, truncation, trailing/concatenated data, invalid lengths, address overflow, resource-limit violations, and output exceeding the advertised bound.
* Reject digest mismatches, missing signatures under signed-node policy, and invalid signatures. No rejected node becomes routable and no failure triggers a silent format or authentication downgrade.
* If caching exists, verify identical logical cache identity across transfer encodings and compression levels. For both unsigned forms, test advertised-digest text and compiled hits with no `FILE_OPEN` or transfer/decoder allocations, plus misses and absent digests. For both signed forms, require transfer and successful verification before a compiled hit bypasses parsing/compilation. Test incompatible or corrupt cache entries and signed-node policy before unsigned lookup.
* Measure concurrent large definitions against the aggregate memory budget. Run the normal unit/integration, ASan/UBSan, and mandatory TSan checks when C implementation changes are made.

Coordinate delivery across the protocol specification, c-apx receiver/client support, py-apx generator support, APX-ES publication metadata, and the affected gateway. A server-only implementation does not itself make compressed generated firmware or legacy gateways compatible.
