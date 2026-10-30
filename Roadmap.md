# c-apx Roadmap

These are the items left to implement for c-apx v0.4.0.

> [!NOTE]
> Detailed design specifications for diagnostics, event streaming, replay, and transport security are maintained under [`design/`](design/):
> * [diagnostics_and_logging_architecture.md](design/diagnostics_and_logging_architecture.md): Overall architecture and RemoteFile Stream File model.
> * [securing_monitor_extension.md](design/securing_monitor_extension.md): Dedicated diagnostic transport (port 5080) and mTLS security.
> * [event_streaming_and_apxlog_spec.md](design/event_streaming_and_apxlog_spec.md): `server.log` wire framing, `.apxlog` format, and `apxlog` CLI tool.
> * [apxmon_diagnostics_spec.md](design/apxmon_diagnostics_spec.md): Diagnostic query-response protocol and `apxmon` CLI tool.
> * [remotefile1_1_stream_file_proposal.md](design/remotefile1_1_stream_file_proposal.md): RemoteFile v1.1 Stream File (`FileStream`) semantics, credit-based flow control, and stream lifecycle.

## 1. Analyze all TODO comments in the c-apx code (Completed)

* [x] **Completed**: All inline TODO comments across the repository have been investigated and resolved:
  * Implemented missing functionality (e.g. `apx_vm_deserializer_unpack_char8`, file manager error handling/cleanup, socket accept cleanup).
  * Removed obsolete, stale, and duplicate comments.
  * Extracted major architectural features into dedicated items in this roadmap (Items 8, 12, 13, 14).

## 2. Event Streaming Protocol & .apxlog Binary Container (apxlog)

* Detailed architecture and wire specifications defined in [event_streaming_and_apxlog_spec.md](design/event_streaming_and_apxlog_spec.md) and [diagnostics_and_logging_architecture.md](design/diagnostics_and_logging_architecture.md).
* **RemoteFile Stream Delivery (`server.log`):**
  * Deliver event stream via virtual stream file `server.log` (RemoteFile type `FileStream` / `RMF_FILE_TYPE_STREAM`), avoiding modifications to low-level RemoteFile wire opcodes.
  * Server announces logging capability on port `5080` / UNIX socket by publishing `server.log`; recorder client subscribes by sending `RMF_CMD_OPEN_FILE`.
  * Lock-free circular ring buffer in `apx_monitor_extension` with drop-counter record (`APX_LOG_EVENT_DROP_COUNT`) on buffer overflow to guarantee zero real-time bus degradation.
* **High-Precision Cross-Platform Timestamps:**
  * Implement portable `apx_get_timestamp_ns()` in `cutil` / `osutil.h` using `GetSystemTimePreciseAsFileTime` (Windows) and `clock_gettime(CLOCK_REALTIME)` (Linux/POSIX) returning nanoseconds since Unix Epoch (1970-01-01).
* **Self-Contained Snapshots & `.apxlog` Binary Format:**
  * Initial state snapshot on file open: active connections, `.apx` definition IDL texts, and current port values.
  * 64-byte file header (`\0APXLOG\1`), append-only sequential event records (24-byte record header + payload in Little-Endian), and optional index trailer for fast random-access seeking.
* **`apxlog` Utility (`app/apx_log`):**
  * `apxlog record -o session.apxlog [--tls]`: Stream live server events directly to disk.
  * `apxlog replay session.apxlog [--exclude-node <name>] [--rate <float>] [--loop]`: Play back recorded scenarios by connecting as a standard multi-node client on operational port `5000` (simulates surrounding nodes for HIL / single-node debugging without server-side modifications).
  * `apxlog info` and `apxlog export --format ndjson`: Metadata inspection and JSON export.

## 3. Complete File Observation in Server Monitor (Completed)

* **Implement Extended File Info (`apx_core`):**
  * Implement `apx_core/src/extended_file_info.c` (constructors, destructors, clone, accessors) and register in `CMakeLists.txt`.
* **Enhance Server Connection Event Listener (`event_listener.h`):**
  * Add `file_opened` and `file_closed` callbacks to `apx_server_connection_event_listener_t`.
  * Add connection-level `file_written` callback (`apx_file_write_event_func_t`) to observe data writes without modifying `apx_file_t` internals.
  * Emit `APX_EVENT_REMOTE_FILE_OPENED` in `file_manager.c` and `server_connection.c`.
* **Complete `observed_file.c` & `observed_file.h`:**
  * Implement `apx_observed_file_create()` / `_destroy()` wrapping `rmf_extended_file_info_t`.
  * Fix syntax typo in `apx_observed_file_delete()` (stray semicolon on `if (self != NULL);`).
* **State & Lifecycle Tracking in `server_monitor.c`:**
  * In `register_connection_listener()`, register `file_published`, `file_opened`, and `file_written`.
  * Manage `apx_observed_file_t` instances in `apx_observed_connection_t->file_list`.
  * Advance `rmf_extended_file_info_t->data_state` through its lifecycle:
    `WAITING_FOR_FILE_OPEN_REQUEST` → `WAITING_FOR_FILE_DATA` → `SYNCHRONIZED`.
* **Unit Tests:**
  * Add test cases to `testsuite_server_monitor_state.c` verifying file creation, state transitions, and file list management.

## 4. RemoteFile Stream File Protocol Implementation (`apx_core`) (Completed)

Detailed protocol specification and state machine definitions are in [remotefile1_1_stream_file_proposal.md](design/remotefile1_1_stream_file_proposal.md).

* [x] **Protocol Commands & Wire Encoders/Decoders (`remotefile.h` / `remotefile.c`):**
  * Implement encoding and decoding for RemoteFile v1.1 stream commands:
    * `RMF_CMD_STREAM_OPEN` (opcode `25u`, 16 bytes): `StartAddress`, `MaxBlockSize`, `WindowBlocks`, `Flags`.
    * `RMF_CMD_STREAM_OPEN_RSP` (opcode `26u`, 20 bytes): `StartAddress`, `Status`, `BlockSize`, `WindowBlocks`, `Flags`.
    * `RMF_CMD_STREAM_WINDOW_UPDATE` (opcode `27u`, 16 bytes): `StartAddress`, `ConsumedBlockCount` (uint64_t).
    * `RMF_CMD_STREAM_ABORT` (opcode `28u`, 12 bytes): `StartAddress`, `Status`.
  * Add unit tests in `testsuite_remotefile.c` covering serialization and deserialization edge cases.
* [x] **Stream Lifecycle & Negotiation (`file_manager.c` / `file_manager_worker.c`):**
  * Support `RMF_FILE_TYPE_STREAM` in file manager registration and publication.
  * Implement receiver-side open request API: `apx_file_manager_open_stream(self, file, max_block_size, window_blocks)`.
  * Implement publisher-side negotiation in worker:
    * `BlockSize = min(FileSize, MaxBlockSize)`.
    * `WindowBlocks = min(requested, max_supported)`.
    * Emit `RMF_CMD_STREAM_OPEN_RSP` and transition stream state to OPEN.
  * Implement clean stream abort and release of queued data without closing the transport connection.
* [x] **Block-Preserving Framing & Reassembly (`file_manager_receiver.c` / `file.c`):**
  * Enforce offset `0` writing constraint for `FileStream`.
  * Accumulate fragmented writes where `MORE_BIT = 1` into a continuous buffer up to negotiated `BlockSize`.
  * On final packet with `MORE_BIT = 0`, dispatch the complete, atomic application block to registered file listener (`apx_file_write_event_func_t`).
* [x] **Cumulative Credit-Based Flow Control:**
  * **Publisher**: Maintain `transmitted_blocks` and `consumed_blocks` counters; pause stream writes when `transmitted_blocks - consumed_blocks >= WindowBlocks`.
  * **Receiver**: Return credit via `RMF_CMD_STREAM_WINDOW_UPDATE` whenever application consumes $\ge 50\%$ of `WindowBlocks`.
* [x] **Testing:**
  * Add unit tests in `testsuite_file_manager.c` verifying stream open handshake, block reassembly with `MORE_BIT`, window exhaustion/unblock, and abort teardown.

## 5. Server Event Tap & Stream Producer (`server.log`) (Completed)

* [x] **Event Tap Integration (`apx_monitor_extension` / `server_monitor.c`):**
  * Hook into server connection and file lifecycle events:
    * `on_connection_accepted` $\to$ emit `EVENT_CONNECTION_CREATE`.
    * `on_connection_disconnected` $\to$ emit `EVENT_CONNECTION_CLOSE`.
    * `on_file_published` $\to$ emit `EVENT_FILE_PUBLISH`.
    * `on_file_revoked` $\to$ emit `EVENT_FILE_REVOKE`.
    * `on_file_written` $\to$ emit `EVENT_FILE_WRITE` with exact timestamp, address, and payload bytes.
  * Fast-path gating (`num_active_log_streams == 0`) ensures zero overhead on the operational bus when no monitor sessions are subscribed.
* [x] **Circular Ring Buffer & Drop-Count Accounting:**
  * Implement a bounded circular ring buffer with drop tracking to isolate the server real-time signal routing thread from the logging transport.
  * When buffer capacity is exceeded, drop incoming events and enqueue an `EVENT_EVENTS_DROPPED` record with drop counts and sequence intervals upon recovery.
* [x] **Snapshot Generation on Subscription:**
  * When a recording client opens `server.log`, generate a coherent initial snapshot sequence:
    `STREAM_BEGIN` $\to$ connected client IDs/tags $\to$ active node `.apx` definitions $\to$ current port values $\to$ `SNAPSHOT_END`.
* [x] **RemoteFile Stream Delivery (`FileStream`):**
  * Publish `server.log` with `FileType = FileStream` (RemoteFile v1.1).
  * Enqueue discrete binary event blocks via `apx_file_manager_send_local_data`.
* [x] **Unit & Integration Tests:**
  * Add unit tests in `testsuite_monitor_extension.c` verifying ring buffer overflow and drop accounting, fast-path tap activation/deactivation, and snapshot generation without memory leaks or race conditions.

## 6. APX Diagnostic Protocol & `apxmon` CLI Application (Retires `apx_info`) (Completed)

* Detailed protocol and CLI design defined in [apxmon_diagnostics_spec.md](design/apxmon_diagnostics_spec.md) and [diagnostics_and_logging_architecture.md](design/diagnostics_and_logging_architecture.md).
* [x] **Retire `apx_info` Prototype & Reconcile Client Abstractions:**
  * Retire throwaway prototype `app/apx_info`.
  * Resolve and reconcile client monitor connection abstractions (`client_monitor_connection.c` / `monitor_socket_client_connection.c`).
* [x] **Bidirectional Diagnostic Streams:**
  * Client publishes `client.request` (`FileStream`); server opens it.
  * Server publishes `server.response` (`FileStream`); client opens it.
  * Multiplexed via 32-bit `request_id` in uniform 16-byte diagnostic header.
* [x] **Server-Side Query Dispatcher (`apx_monitor_extension`):**
  * `APX_DIAG_CMD_GET_SERVER_INFO`: Version, uptime, active connection and node counts.
  * `APX_DIAG_CMD_GET_CLIENTS` / `APX_DIAG_CMD_GET_NODES`: Detailed connection tables and registered nodes.
  * `APX_DIAG_CMD_GET_NODE_DEF`: Fetches complete `.apx` IDL text on demand.
  * `APX_DIAG_CMD_GET_ROUTING_TABLE`: Full port routing map (Provide Node:Port $\to$ Require Node:Port).
  * `APX_DIAG_CMD_GET_UNCONNECTED_PORTS`: Detects dangling require ports without providers and unconsumed provide ports.
  * `APX_DIAG_CMD_WATCH_PORT` / `APX_DIAG_NOTIF_PORT_UPDATE`: Live signal tapping with asynchronous updates.
* [x] **`apxmon` CLI Application (`app/apx_mon`):**
  * Subcommands: `info`, `clients`, `nodes`, `routing [--unconnected]`, `get-apx <node>`, `dump <node>`, `watch <node:port>`, and interactive `top`.
  * Supports aligned formatted ASCII table output and machine-readable `--json` output.

## 7. Dedicated Diagnostic Transport & TLS Security for Server Monitor (Completed)

* Detailed architecture and design defined in [securing_monitor_extension.md](design/securing_monitor_extension.md).
* [x] **Dedicated Listener Integration (`apx_monitor_extension`):**
  * Decouple monitor connections from `apx_socket_extension` so the monitor extension manages its own dedicated `msocket_server_t`.
  * Support listening on a dedicated UNIX domain socket (e.g. `/run/apx/apx_monitor.socket`) and/or dedicated TCP port (default: 5080, following the convention where `apx-server@5000+N` maps to diagnostic port `5080+N`) configured via `server.json`.
  * Automatically instantiate accepted connections directly as `APX_CONNECTION_TYPE_MONITOR` and publish `server.log` and `server.response`.
* [x] **Mbed TLS Transport Security (mTLS / PSK):**
  * Integrate `msocket_tls_config_t` into `apx_server_monitor_t` to support TLS on the diagnostic TCP port.
  * Enforce Mutual TLS (`require-client-cert: true`) so only authorized diagnostic clients possessing a valid client certificate can connect.
  * Verify client certificate Subject / Common Name to prevent standard node certificates from connecting to the monitor endpoint.
* [x] **Client Tooling Connection Support:**
  * Add TLS connection arguments (`--tls`, `--ca-cert`, `--client-cert`, `--client-key`) and default port/socket (`5080` / `/run/apx/apx_monitor.socket`) to `apxmon` and `apxlog`.

## 8. Implement Worker Command Stubs (`file_manager_worker.c`) (Completed)

* [x] Implement `APX_CMD_REVOKE_LOCAL_FILE` (completed).
* [x] Implement `APX_CMD_CLOSE_REMOTE_FILE` (completed).
* [x] Implement `APX_CMD_SEND_ERROR_CODE` (completed).

## 9. Shared Library Symbol Visibility and Windows DLL Export

Control symbol visibility in `libapx_core` shared library builds and enable Windows DLL support.

* **API Export Macro (`APX_API` / `DLL_PUBLIC`):**
  * Establish a standard export/import macro header (e.g. finalizing `DLL_PUBLIC` in `apx/types.h` or creating `apx/export.h`).
  * Linux/GCC/Clang: `__attribute__((visibility("default")))` when building the shared library.
  * Windows/MSVC: `__declspec(dllexport)` when compiling `apx_core` as a DLL, and `__declspec(dllimport)` when consumed by external applications/tests.
  * Set `DLL_LOCAL` to `__attribute__((visibility("hidden")))`.
* **Annotate Public Header APIs:**
  * Decorate all public API functions in `apx_core/include/apx/*.h` with the export macro.
  * Keep internal symbols (including bundled submodule functions from `adt`, `cutil`, `dtl_type`, `dtl_json`, and `msocket`) hidden within the shared library.
* **CMake Configuration:**
  * Uncomment `target_compile_options(apx_core PRIVATE -fvisibility=hidden)` in `CMakeLists.txt` for ELF/Linux builds.
  * Enable Windows DLL builds (`BUILD_SHARED_LIBS=ON`) once MSVC export definitions are in place.

## 10. Support Newline-Delimited JSON (NDJSON) in `apx-node` JSON Message Server

Currently, `app/apx_node/src/json_server_connection.c` expects every message to be framed using a binary `numheader32` length prefix. This prevents generic tools (such as `nc`, `socat`, or standard shell pipelines) from sending raw JSON directly (e.g. `echo '{"VehicleSpeed": 72.5}' | nc -U /tmp/apx_node.socket` fails because `{` / `0x7B` is interpreted as a binary length byte).

* **Auto-Detection / Multi-Framing Support:**
  * In `json_server_connection_data()`: Inspect the initial byte of the incoming buffer.
  * If the first byte is `{` or whitespace (`0x20`, `\t`, `\r`, `\n`), parse the stream using newline delimitation (`\n` / `\r\n`) or balanced-brace boundaries.
  * If the first byte is a binary `numheader` byte, preserve the existing `numheader_decode32()` framing for backward compatibility with `apx_control`.
* **Testing & Tooling:**
  * Enable direct shell integration and piping from `nc -U`, `socat`, and standard POSIX utilities without needing helper wrappers.
  * Add pytest integration test verifying NDJSON updates sent via raw socket.

## 11. Modernize TextLog Extension & Logging Infrastructure (Completed)

The `c-apx` codebase contains numerous compile-time `#if APX_DEBUG_ENABLE` checks and raw `printf` statements across server and connection components. While `apx_log_level_t` exists in `apx_core/include/apx/types.h` and `apx_server_log_write()` is implemented, the `apx_text_log_extension` ignores log levels and lacks runtime level filtering.

* [x] **TextLog Extension Configuration & Level Filtering:**
  * Support a `"log-level"` setting (values: `"CRITICAL"`, `"ERROR"`, `"WARNING"`, `"INFO"`, `"DEBUG"`; default: `"INFO"`) in `server.json` under `"textlog-extension"`.
  * Add string conversion utilities (`apx_log_level_from_string()` / `apx_log_level_to_string()`).
  * Add `log_level` to `apx_text_log_base_t` / `apx_server_text_log_t` and filter incoming log events so only messages with `level <= configured_level` are output.
* [x] **Standardized Log Message Formatting:**
  * Support a `"use-timestamp"` boolean setting (default: `false`) in `server.json` under `"textlog-extension"`.
  * Format output with consistent prefixes: `[<timestamp>] [<LEVEL>] [<LABEL>] <message>` (or `[<LEVEL>] [<LABEL>] <message>` when timestamps are disabled) with fixed-width padded levels and labels for column alignment.
  * Classify built-in server events in `server_text_log.c` with appropriate log levels instead of direct unlevelled prints (e.g., connections/disconnections and file publishes as `INFO`, port routing and protocol header handshakes as `DEBUG`).
* [x] **Replace Legacy Server `APX_DEBUG_ENABLE` Prints:**
  * Replace compile-time `#if APX_DEBUG_ENABLE` prints in server modules (`server_connection.c`, `connection_manager.c`, `socket_server_connection.c`, `socket_server.c`, `tls_server.c`, `tls_server_connection.c`) with `apx_server_log_write(server, APX_LOG_LEVEL_DEBUG, ...)`.
  * Ensure high-frequency debug logging (such as packet reception/transmission) does not exhaust server event loop memory via early check on active log listeners before event allocation.
* [x] **Complete Syslog Integration (`syslog-enabled`):**
  * Wire up `syslog-enabled` and `syslog-label` in `server_text_log_extension.c` and `text_log_base.c` using POSIX `syslog(priority, ...)`, mapping `apx_log_level_t` directly to syslog priorities (`LOG_CRIT`, `LOG_ERR`, `LOG_WARNING`, `LOG_INFO`, `LOG_DEBUG`).

## 12. Small Data Support in Command Pipeline

In `apx_command_t` (`apx_core/include/apx/command.h`), the union `msgData3_tag` includes an inline buffer `uint8_t data[APX_SMALL_DATA_SIZE]` (where `APX_SMALL_DATA_SIZE` is 8 bytes, defined in `cfg.h`). This is designed to optimize transmission of small port/signal data (scalar numeric types, booleans, and small byte arrays) without requiring dynamic heap allocation.

Currently, all local data transmission via `APX_CMD_SEND_LOCAL_DATA` treats `data3.ptr` as a heap-allocated pointer:
* Callers in `apx_nodeInstance_t` dynamically allocate memory before calling `apx_file_manager_send_local_data()`.
* `run_send_local_data()` and `cleanup_cmd_queue()` unconditionally call `free(data)` / `free(cmd.data3.ptr)`.

To eliminate heap allocation and deallocation overhead for high-frequency signal updates:

* **Command Type Distinction:**
  * Add a dedicated command opcode (e.g. `APX_CMD_SEND_LOCAL_SMALL_DATA`) or a discriminator flag to `apx_command_t`.
* **Worker Queue Handling (`file_manager_worker.c`):**
  * Implement `apx_file_manager_worker_prepare_send_local_small_data(self, address, data, size)` using `apx_build_command_with_data()`.
  * In `process_single_command()`: dispatch `APX_CMD_SEND_LOCAL_SMALL_DATA` to transmit without freeing `data3`.
  * In `cleanup_cmd_queue()`: bypass `free()` for small data commands.
* **File Manager API (`file_manager.c`):**
  * Add `apx_file_manager_send_local_small_data(self, address, const uint8_t* data, apx_size_t size)`.
* **Node Instance Integration (`node_instance.c`):**
  * When publishing port changes (`node_instance.c:2100` / `//TODO: use small object allocator later on`), check if payload size `<= APX_SMALL_DATA_SIZE`.
  * If small, copy directly into stack buffer and route via `apx_file_manager_send_local_small_data()`, avoiding heap allocation entirely.

## 13. Support Multiple Providers per Require Port

In `apx_node_instance_handle_require_ports_disconnected` (`apx_core/src/node_instance.c`), the connector change table supports disconnect transitions where `entry->count == -1` (single provider disconnection). When `entry->count < -1`, the function currently returns `APX_NOT_IMPLEMENTED_ERROR`.

In `apx_port_connector_change_entry_t` (`apx_core/include/apx/port_connector_change_entry.h`), `union portref_union_tag` already provides an `adt_ary_t *array` when `count < -1` or `count > 1`, designed to hold multiple provider references.

* **Multi-Provider Disconnect Handling (`node_instance.c`):**
  * When `entry->count < -1`, iterate over all disconnected provide ports stored in `entry->data.array` using `apx_port_connector_change_entry_get(entry, i)`.
  * For each provide port, acquire `apx_node_instance_lock_port_connector_table(provide_node_instance)`, invoke `remove_provide_port_connector()`, compute the updated provide port count, and unlock.
  * Send provide port count data notifications for all affected provide node instances.
  * Send require port count update for the require port.
* **Testing:**
  * Add unit and integration tests verifying multi-provider connect and simultaneous disconnect transitions.

## 14. Node Definition Cache (Text & Binary Form)

Implement disk-based caching similar to Python's `.py` / `.pyc` model to eliminate definition transfer overhead and bytecode compilation during ECU startup and wake-from-sleep.

### Background & Motivation
In automotive production systems, ECUs frequently wake from sleep and restart the APX network. Transferring `.apx` definition files over IPC/network on every wake-up consumes communication bandwidth, and compiling definitions using the APX IDL parser and VM compiler incurs significant CPU latency. Because node definitions only change during workshop software flashing or remote OTA updates, caching definitions locally yields near-instantaneous node initialization.

The SHA-256 checksum in `rmf_file_info_t` enables zero-overhead, content-addressable cache lookups and automatic cache invalidation.

### Cache Hierarchy & Resolution Order
When a remote node publishes its `.apx` definition file with a valid SHA-256 digest:

1. **Tier 1: Level 2 (Binary Bytecode) Cache Hit** (Fastest: zero network, zero compilation)
   * Check for precompiled binary cache (`<cache_dir>/bin/<node_name>_<sha256>.apxc`).
   * If valid: deserialize port instances, VM pack/unpack bytecode programs, port signatures, and init data directly into `apx_node_instance_t`.
   * Set `*file_open_request = false` in `apx_node_manager_remote_file_published_notification`.
   * In `server_connection.c`, transition definition data state directly to `APX_DATA_STATE_SYNCHRONIZED` and immediately open data exchange files (`.in` / `.out`).
2. **Tier 2: Level 1 (Text Definition) Cache Hit** (Zero network, local compilation)
   * If binary cache misses or is invalid, check `<cache_dir>/text/<node_name>_<sha256>.apx`.
   * If found: read `.apx` text from disk (`*file_open_request = false`), parse and compile into `apx_node_instance_t`.
   * Write compiled output to Level 2 binary cache for subsequent boots.
3. **Tier 3: Cache Miss** (Cold boot / First run / OTA update / Missing digest)
   * If both caches miss or no digest was provided, set `*file_open_request = true`.
   * Transfer definition over the wire $\to$ save raw text to Level 1 $\to$ compile $\to$ save binary to Level 2.

### Interaction with Cryptographically Signed Nodes (RMFP 1.1)
* **Mandatory File Transfer for Signature Verification**:
  Because `RMF_CMD_PUBLISH_SIGNED_FILE` only transmits the public signature and not an independent digest, the server must never bypass file download for signed nodes (`*file_open_request = true`). The server must receive the actual definition bytes over the wire to cryptographically verify them against trusted public keys via `apx_server_verify_node_signature()`. Skipping file transfer based solely on a replayed public signature would allow unauthorized clients to spoof node identities.
* **Compilation Caching Still Active**:
  Once the received definition is cryptographically verified against the public keys, the server computes its SHA-256 and checks the Level 2 binary cache (`.apxc`). If present, IDL parsing and VM bytecode compilation are completely bypassed, eliminating CPU overhead on startup while preserving full cryptographic integrity.

### Level 2 Binary Format (`.apxc`) Specification
* **Magic & Header**: 4-byte magic (`\0APX`), binary format version, compiler/VM bytecode version, node definition SHA-256 digest, and node name.
* **Port Descriptors**:
  * Port IDs, names, directions, and data offsets/sizes.
  * Serialized pack and unpack bytecode programs (`apx_program_t`).
  * Port signature strings/hashes (required for server port routing and matching).
* **Port Init Data**: Pre-serialized binary buffer for provide and require initial port values.

### Reliability & Embedded Considerations
* **Atomic Cache Writes**: Write new cache files to a temporary file (`.tmp`) and atomically `rename()` to prevent partial or corrupted cache files on sudden power loss.
* **Resilient Fallback**: Corrupted or version-mismatched binary cache files transparently fall back to Level 1 or wire download without failing node connection.
* **Pre-Warming**: Offline tools or OTA post-install scripts can pre-populate `.apxc` cache files during vehicle software installation.

## 15. Configuration Validation & Unknown Key Diagnostics

Currently in `server_cfg.c` and extension initialization routines (e.g. `server_text_log_extension.c`, `socket_server_extension.c`), configuration parameters are looked up on demand via `dtl_hv_get_cstr(cfg, "key")`. If a user misspells a configuration key (e.g. `"use-time-stamp"`, `"log_lvl"`, or `"prot"` instead of `"tcp-port"`), the key is silently ignored, and the system falls back to default values without any warning or operator feedback.

To improve usability and prevent silent configuration errors:

* **Unknown Key Detection & Warning:**
  * Implement an allowlist or schema validation helper for dynamic type hashes (e.g. `cutil` / `dtl` hash inspection or a schema checker function `apx_config_validate_keys()`).
  * Iterate over all keys present in the loaded JSON object/hash:
    * Emit a warning (via standard error or `apx_server_log_write()` at `APX_LOG_LEVEL_WARNING`) for any unrecognized key in `server.json` top-level sections (e.g. `apx-server`, `socket-server-extension`, `textlog-extension`, etc.).
    * Emit section-level warnings for unrecognized keys within each extension's configuration block.
* **Typo Distance Suggestion (Optional Enhancement):**
  * Use Levenshtein distance to suggest intended keys for minor typos (e.g., *"Unknown key 'use-timestmp'; did you mean 'use-timestamp'?"*).
* **Strict Validation Option:**
  * Provide a `--strict-config` CLI flag or `"strict-config": true` option to reject unknown keys and abort server startup with `APX_INVALID_ARGUMENT_ERROR`, preventing unintended fallback behavior in automated deployment or test environments.

## 16. Compressed Transfer for Signed APX Definitions (RMFP 1.1)

To reduce wire transfer latency for signed APX nodes by >80% during startup phase while maintaining strict cryptographic verification:

* **Protocol Extension (`RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE_MSG` / Opcode `24u`):**
  * Define opcode `24u` in `apx_core/include/apx/remotefile.h` with an 88-byte fixed header:
    * `cmd_type` (`uint32_t`, 24u)
    * `address` (`uint32_t`, base memory address)
    * `uncompressed_size` (`uint32_t`, decompressed size of `.apx` text)
    * `compressed_size` (`uint32_t`, total bytes streamed across the wire)
    * `file_type` (`uint16_t`, `RMF_U16_FILE_TYPE_FIXED`)
    * `compression_type` (`uint16_t`, `1u` = Deflate)
    * `sig_type` (`uint16_t`, `1u` = ECDSA-P256)
    * `reserved` (`uint16_t`, 0u for 32/64-bit alignment)
    * `signature_data` (`uint8_t[64]`, ECDSA-P256 signature of uncompressed text)
    * `name` (`char[]`, null-terminated file name)
  * Implement encoder and decoder functions in `remotefile.c` / `remotefile.h`.
* **In-Memory Decompression with `miniz`:**
  * Integrate single-file, zero-dependency C99 Deflate library (`miniz`).
  * In `apx_file_manager_receiver_t`, buffer the incoming `compressed_size` bytes.
  * Upon transfer completion, decompress in-memory into an `uncompressed_size` buffer (~0.3 ms for 140 KB definitions).
* **Cryptographic Verification & Cache Integration:**
  * Cryptographically verify the uncompressed buffer against `signature_data` via `apx_server_verify_node_signature()`.
  * Compute SHA-256 of verified text and query Level 2 binary cache (`.apxc` from Item 14) to skip IDL parsing and VM bytecode compilation entirely.
* **Backward Compatibility Fallback:**
  * If a legacy server returns `RMF_CMD_NACK_MSG` on opcode 24, the client falls back to uncompressed `RMF_CMD_PUBLISH_SIGNED_FILE_MSG` (opcode 23).



