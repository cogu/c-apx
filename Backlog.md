# c-apx Backlog

> [!NOTE]
> Detailed design specifications for diagnostics, event streaming, replay, transport security, and routing policies are maintained under [`design/`](design/):
> * [diagnostics_and_logging_architecture.md](design/diagnostics_and_logging_architecture.md): Overall architecture and RemoteFile Stream File model.
> * [securing_monitor_extension.md](design/securing_monitor_extension.md): Dedicated diagnostic transport (port 5080) and mTLS security.
> * [event_streaming_and_apxlog_spec.md](design/event_streaming_and_apxlog_spec.md): `server.log` wire framing, `.apxlog` format, and `apx-log` CLI tool.
> * [apxmon_diagnostics_spec.md](design/apxmon_diagnostics_spec.md): Diagnostic query-response protocol and `apx-mon` CLI tool.
> * [small_data_and_allocator_spec.md](design/small_data_and_allocator_spec.md): Inline small data pipeline (`APX_SMALL_DATA_SIZE`), small object allocator (`soa`), and event queue configuration.
> * [multiple_providers_policy_spec.md](design/multiple_providers_policy_spec.md): Multiple port providers arbitration policy (`"first"` vs. `"last"`), provider state machine, and observability.

---

# Release v0.4.0 (Current Phase)

These are the items left to implement for c-apx v0.4.0.

## 1. Analyze all TODO comments in the c-apx code (Completed)

* [x] **Completed**: All inline TODO comments across the repository have been investigated and resolved:
  * Implemented missing functionality (e.g. `apx_vm_deserializer_unpack_char8`, file manager error handling/cleanup, socket accept cleanup).
  * Removed obsolete, stale, and duplicate comments.
  * Extracted major architectural features into dedicated items in this backlog (Items 8, 12, 13, 14).

## 2. Event Streaming Protocol & .apxlog Binary Container (apx-log) (Completed)

* Detailed architecture and wire specifications defined in [event_streaming_and_apxlog_spec.md](design/event_streaming_and_apxlog_spec.md) and [diagnostics_and_logging_architecture.md](design/diagnostics_and_logging_architecture.md).
* [x] **RemoteFile Stream Delivery (`server.log`):**
  * Deliver event stream via virtual stream file `server.log` (RemoteFile type `FileStream` / `RMF_FILE_TYPE_STREAM`), avoiding modifications to low-level RemoteFile wire opcodes.
  * Server announces logging capability on port `5080` / UNIX socket by publishing `server.log`; recorder client subscribes by opening the stream.
  * Circular ring buffer in `apx_monitor_extension` with drop-counter record (`APX_LOG_EVENT_EVENTS_DROPPED`) on buffer overflow to guarantee zero real-time bus degradation.
* [x] **High-Precision Cross-Platform Timestamps:**
  * Portable `apx_get_unix_time_ns()` and `apx_get_monotonic_time_ns()` in `apx_core` / `apx/util.h` returning nanoseconds.
* [x] **Self-Contained Snapshots & `.apxlog` Binary Format:**
  * Initial state snapshot on stream open: active connections, `.apx` definition IDL texts, and published files/ports.
  * 64-byte file header (`\0APXLOG\n`), append-only sequential event records (canonical NumHeader32 length prefix + 28-byte event header + payload in Little-Endian).
* [x] **`apx-log` Utility (`app/apx_log`):**
  * `apx-log record [OPTIONS] <FILE>`: Stream live server events directly to disk into `.apxlog` binary container format.
  * `apx-log info [OPTIONS] <FILE>`: Metadata inspection and file summary (timestamps, duration, record count, file size, optional `--json`).
  * `apx-log print [OPTIONS] <FILE>`: Sequential event stream decoding and console display (formatted text and `--json` NDJSON output).

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

Detailed protocol specification and state machine definitions are formalized in the normative RemoteFile v1.1 specification (`remotefile1_1.md`).

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

## 6. APX Diagnostic Protocol & `apx-mon` CLI Application (Retires `apx_info`) (Completed)

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
* [x] **`apx-mon` CLI Application (`app/apx_mon`):**
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
  * Add TLS connection arguments (`--tls`, `--ca-cert`, `--client-cert`, `--client-key`) and default port/socket (`5080` / `/run/apx/apx_monitor.socket`) to `apx-mon` and `apx-log`.

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

## 10. Switch `apx-node` and `apx-control` to Newline-Delimited JSON (NDJSON) (Completed)

Previously, `app/apx_node/src/json_server_connection.c` and `app/apx_control/src/message_client_connection.c` framed JSON messages using a binary `numheader32` length prefix ("length-prefixed JSON"). This prevented generic tools (such as `nc`, `socat`, or standard shell pipelines) from sending raw JSON directly (e.g., `echo '{"VehicleSpeed": 72.5}' | nc -U /tmp/apx_node.socket` failed because `{` was interpreted as a binary length header).

* [x] **Retire Length-Prefixed (`numheader32`) JSON in `apx-node` JSON Message Server:**
  * Remove `numheader` binary framing completely from `json_server_connection.c`.
  * Support pure Newline-Delimited JSON (NDJSON): ingest stream by scanning for `\n` line delimiters, stripping trailing `\r` (`\r\n`), and parsing each line with `dtl_json_load_bstr()`.
* [x] **Update `apx-control` to NDJSON:**
  * Update `message_client_prepare_message()` to send raw JSON terminated with `\n` without binary length headers.
* [x] **Testing & Tooling:**
  * Enable direct shell integration and piping from `nc -U`, `socat`, and standard POSIX utilities without needing helper wrappers.
  * Add pytest integration tests verifying NDJSON updates via raw socket streaming and `apx-control`.

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

## 12. Small Data Support in Command Pipeline & Retiring `apx_allocator_t` (Completed)

* Detailed historical analysis and 3-tier architecture defined in [small_data_and_allocator_spec.md](design/small_data_and_allocator_spec.md).

In `apx_command_t` (`apx_core/include/apx/command.h`), the `data3` union overlays `void *ptr` with an inline buffer `uint8_t data[APX_SMALL_DATA_SIZE]` (`8` bytes). Currently, all local data transmission via `APX_CMD_SEND_LOCAL_DATA` treats `data3.ptr` as a heap-allocated pointer (`malloc` / `free`), while `apx_connection_base_t` spawns an unused `apx_allocator_t` wrapper and background thread on every connection.

To eliminate heap allocation overhead and per-connection garbage-collector threads, retire `allocator.c` (`apx_allocator_t`) and maintain `soa_t` directly inside `apx_file_manager_worker_t` (protected by `self->queue_lock`) with a 3-tier allocation pipeline:

1. **Tier 1 — Inline Small Data (`1 .. 8` bytes, `size <= APX_SMALL_DATA_SIZE`):**
   * Enqueued as `APX_CMD_SEND_LOCAL_SMALL_DATA` with payload bytes embedded directly in `cmd.data3.data[0..7]` (zero allocations, zero frees).
2. **Tier 2 — Small Data via SOA (`9 .. 32` bytes, `APX_SMALL_DATA_SIZE < size <= SOA_SMALL_OBJECT_MAX_SIZE`):**
   * Enqueued as `APX_CMD_SEND_LOCAL_SOA_DATA` with `cmd.data3.ptr` allocated from `worker->soa` (`soa_alloc()`) and freed via `soa_free()` under `self->queue_lock`.
3. **Tier 3 — Normal / Large Data (`> 32` bytes, `size > SOA_SMALL_OBJECT_MAX_SIZE`):**
   * Enqueued as `APX_CMD_SEND_LOCAL_DATA` with `cmd.data3.ptr` allocated on the heap via `malloc()` and freed via `free()`.

* [x] **Retire `apx_allocator_t` (`allocator.c`, `allocator.h`, `testsuite_allocator.c`):**
  * Remove `allocator.c`, `allocator.h`, and `testsuite_allocator.c` from `apx_core`.
  * Remove `apx_allocator_t allocator` from `apx_connection_base_t` and remove the `allocator` parameter from `apx_file_manager_create()` and `apx_file_manager_shared_create()`.
* [x] **Command Definition, Documentation & Builder Fix (`command.h`, `command.c`):**
  * Add `APX_CMD_SEND_LOCAL_SMALL_DATA` and `APX_CMD_SEND_LOCAL_SOA_DATA` to `apx_cmd_type_t`.
  * Add a `"Command field usage and ownership"` comment block to `command.h` (matching the `"Event field usage and ownership"` documentation in `event.h`) documenting `data1`, `data2`, `data3`, and ownership semantics for every `APX_CMD_*`.
  * Fix `apx_build_command_with_data()` in `command.c` to copy `d2` (`size`) bytes (`d2 <= APX_SMALL_DATA_SIZE`) rather than unconditionally reading `APX_SMALL_DATA_SIZE` bytes from `d3`.
* [x] **File Manager Worker & 3-Tier Dispatch (`file_manager_worker.c`, `file_manager.c`, `node_instance.c`):**
  * Embed `soa_t soa;` in `apx_file_manager_worker_t` (`soa_init` on create, `soa_destroy` on destroy).
  * Implement `apx_file_manager_worker_prepare_send_local_small_data()` and `apx_file_manager_worker_prepare_send_local_soa_data()`, and handle both in `process_single_command()` and `cleanup_cmd_queue()`.
  * Add `apx_file_manager_write_local_data(self, address, const uint8_t *data, apx_size_t size)` in `file_manager.c` to automatically select Tier 1 (`<= 8` B), Tier 2 (`9..32` B), or Tier 3 (`> 32` B), and call it from `remote_route_data_to_file()` in `node_instance.c`.

## 13. Support Multiple Providers per Require Port (`multiple-provider-policy`)

**Status**: Completed

* Detailed architecture, arbitration state machine, and observability specifications defined in [multiple_providers_policy_spec.md](design/multiple_providers_policy_spec.md).
* **Configurable Provider Arbitration Policy (`server.json` / `server_cfg.c` / `server.c`):**
  * Support `"multiple-provider-policy": "first" | "last"` (with alias `"multiple_provider_policy"`, default: `"first"`) in the `"apx-server"` section of `server.json`.
  * `"first"` policy (Production default): The oldest connected provider (`adt_list_first(&entry->provide_ports)`) wins and retains ownership. Newer providers attach to `entry->provide_ports` as shadowed without disturbing active routing. If the active provider disconnects, the next oldest provider in the list becomes active.
  * `"last"` policy (`v0.2.8` / Debug override mode): The newest connected provider (`adt_list_last(&entry->provide_ports)`) immediately takes over from the previous active provider (which becomes shadowed). If the active provider disconnects, the preceding provider at the tail of `entry->provide_ports` becomes active.
* **Single Active Provider Invariant & Provider Queue (`port_signature_map.c` / `port_signature_map_entry.c`):**
  * Maintain all connected providers for a port signature in chronological order in `entry->provide_ports` (`adt_list_t`), with `entry->preferred_provider` tracking the single active winner.
  * Ensure only `entry->preferred_provider` holds require port references in its `node_instance->connector_table` so shadowed providers never contend with the active provider on `.out` writes.
  * Fix `notify_provide_ports_about_require_port_change()` so newly connecting/disconnecting require ports only bind to `entry->preferred_provider` rather than all `entry->provide_ports`.
* **Two-Phase Lock-Free Takeover & Disconnect Switch (`server.c` / `server_connection.c` / `node_instance.c`):**
  * On `"last"` policy takeover in `apx_server_insert_node_instance()`: disconnect the replaced `old_preferred` provider (`ProvidePortCount = 0`), connect `new_preferred`, and route its current `.out` value to all require ports outside `server->global_lock`.
  * On active provider disconnect in `detach_all_nodes()`: detach all nodes belonging to the closing connection first, select the next surviving provider (`new_preferred`) according to the active policy, and route `new_preferred`'s current `.out` value to surviving require ports outside `server->global_lock`.
  * Complete the `entry->count < -1` loop in `apx_node_instance_handle_require_ports_disconnected()` (`node_instance.c`).
* **Observability (`apx_text_log_extension`, `apx-log`, `apx-mon`):**
  * Emit a server event (`provider_changed`) whenever a port's active provider changes due to takeover or disconnect.
  * `apx_text_log_extension`: Log active provider changes at `INFO` level matching `apx-mon` formatting (`[INFO ] [PORT] Provider changed: 1:NodeA/Port -> 4:NodeB/Port`).
  * `apx-log` / `.apxlog`: Emit `APX_LOG_EVENT_PROVIDER_CHANGE` (`0x0050`) into `server.log` and format in `apx-log print` (text and `--json`). Do not increment the `.apxlog` / `server.log` format minor version (`v1.0` remains unchanged).
  * `apx-mon`:
    * Display `(shadowed)` in the `REQUIRER_NODE` column of `apx-mon routing --unconnected` and `apx-mon routing --all` for shadowed providers (`reason = 2: SHADOWED`).
    * Add `apx-mon routing --multi-provider` (`APX_DIAG_CMD_LIST_MULTI_PROVIDERS`) to report all port signatures with multiple providers (`num_providers > 1`), showing each provider and its status (`ACTIVE` vs. `(shadowed)`).
* **Testing:**
  * Add unit and integration tests verifying multi-provider connect, takeover, shadowed disconnect, active disconnect transitions (e.g. `[1, 4, 5, 7]`), and `apx-mon routing --multi-provider` under both `"first"` and `"last"` policies.

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

## 15. Unknown Configuration Key Warnings (Completed)

Prevent misspelled or unrecognized JSON configuration keys from being silently ignored.

* Warn about unknown top-level configuration section names and unknown keys within recognized server and extension sections, including nested configuration objects.
* Warn when an enabled extension has missing minimum configuration (e.g., `socket-server-extension` enabled with no listening transports, or `monitor-extension` enabled with no endpoints).
* Identify the unrecognized section or key and its configuration path in each warning (e.g., `textlog-extension.use-timestmp`).
* Make warnings visible during startup without depending on the text logging extension being enabled.
* Warn and continue: unknown keys remain ignored, and existing startup and default-value behavior is unchanged.
* Recognize existing supported aliases and registered extension section names. Omitted optional settings continue to use their defaults without warnings.
* Emit unknown-configuration warnings directly to stderr, independently of extension initialization, logging destinations, and log-level filtering.

**Out of scope:** Typo suggestions, Levenshtein distance, strict mode, new CLI/configuration options, and a general schema or value/type-validation framework.

**Acceptance criteria:** Tests cover warnings for unknown sections and keys, continued startup after these warnings, and no unknown-key warnings for recognized keys, supported aliases, or omitted optional settings.

## 16. Compressed Transfer of APX Node Definitions (Signed and Unsigned)

**Status:** Completed. Explicit C client compression selection, argument-free `apx-node --compressed`, receiver validation/resource bounds, and logical definition/rejection logging are implemented and verified with both backends.

**Depends on:** [Item 20: Compression Backend and CMake Integration](#20-compression-backend-and-cmake-integration).

Detailed requirements, protocol rationale, compatibility rules, and acceptance tests are in the [compressed definition transfer design proposal](design/compressed_definition_transfer_spec.md). Wire layouts are specified in [RemoteFile v1.1](../apx/specifications/protocols/remotefile1_1.md#compressed-definition-transfer).

* Support unsigned and signed definitions using zlib-wrapped DEFLATE (RFC 1950 / RFC 1951). Preserve every RMFP 1.0 opcode and the legacy unsigned/uncompressed publication at ID 3.
* Clean up the RemoteFile implementation layer to match the revised RMFP 1.1 allocation: keep header acceptance at 20, group signed/compressed publication at 21-23, and move stream control to 24-27. Remove obsolete `RMF_CMD_CONNECTION_CREATE`, `RMF_CMD_CONNECTION_REVOKE`, `RMF_CMD_PUBLISH_REMOTE_FILE_MSG`, and `RMF_CMD_PUBLISH_SIGNED_REMOTE_FILE_MSG` definitions and unused helpers/tests. Align encoders/decoders, dispatch, and wire-format fixtures; retain independent application event IDs unchanged.
* Add an explicit definition-compression setting to the C client API, accepting a separate enable/disable choice and level `-1` (backend default) or `0` through `9`. Add an argument-free `--compressed` flag to `apx-node`, selecting the backend default (currently level 6). Uncompressed publication remains the default; level 0 still produces a zlib stream. Honor the application's choice without size heuristics, automatic representation selection, or fallback. A future CLI `--compression-level=<level>` is not required for this item.
* Keep the original logical definition for parsing, inspection, hashing, and signing, and prepare a separate compressed transfer buffer before publication. Compression applies only to APX definition files; signal data, initial values, and port-count data remain uncompressed.
* Bound and validate decompression before parsing; verify signatures and optional digests over the exact uncompressed definition bytes. With item 14's optional cache, either unsigned publication may skip transfer on an advertised SHA-256 cache hit; either signed publication must transfer and verify before reusing compiled content. Compression itself does not require caching.
* Use item 20's internal compression/decompression API without library-specific calls in the RemoteFile layer. Dynamic C clients compress definitions before publication; receivers use bounded decompression. The wire format remains independent of the selected backend.
* Normalize successful monitor/log definition events to validated uncompressed text and logical sizes, consistently in live capture and snapshots. Emit `NODE_DEFINITION_REJECTED` for failed plain or compressed attempts, preserving connection identity, encoding, advertised sizes, failure stage, detected error, and intended NACK code. Keep real RemoteFile address ranges and sizes encoded. See the [event-stream contract](design/event_streaming_and_apxlog_spec.md#661-node_definition_rejected).

**Acceptance criteria:** Tests cover opcode cleanup and renumbering with unique RMFP 1.1 IDs and unchanged RMFP 1.0 IDs, signed/unsigned and compressed/uncompressed C client/server operation, explicit API and `--compressed` selection with the existing uncompressed default, fragmented transfers, malformed input, resource limits, cleanup, unchanged signature policy, and the definition/logging contract. Sender tests cover every API level (`-1` and `0` through `9`), actual CLI publication metadata/payloads, signed-node admission, small definitions that grow under compression, and ordinary signal-data behavior with either definition representation.

**Out of scope:** Backend implementation and CMake dependency setup (item 20), automatic legacy fallback/capability negotiation, signal/initial-value/port-count compression, and implementation of the node-definition cache. py-apx and APX-ES will be developed together as part of the ecosystem work for c-apx v0.6.0. Generator, other-language client, gateway, broader interoperability/lifecycle acceptance, and performance benchmark work are not completion criteria for this item.

## 17. Retire Unused `"max-num-events"` Configuration & `APX_MAX_NUM_EVENTS`

* **Status**: Completed

The JSON configuration setting `"max-num-events"` and the compile-time macro `APX_MAX_NUM_EVENTS` (`apx_core/include/apx/cfg.h:49`) are unused leftovers from an early `v0.3.x` refactoring (`commit 1c8b833`, Sep 2019) when `apx_server_t` briefly accepted a `maxNumEvents` cap for a server-level ringbuffer.

* **Core Header Cleanup (`apx_core/include/apx/cfg.h`):**
  * Remove unused `#define APX_MAX_NUM_EVENTS 1000`.
* **Server Configuration & Tests (`app/apx_server`, `tests/`):**
  * Remove `"max-num-events"` from `KNOWN_SERVER_KEYS` in `app/apx_server/src/server_cfg.c`.
  * Remove `"max-num-events"` assertions and test fixtures from `app/apx_server/test/testsuite_server_cfg.c` and `tests/test_server_config_warnings.py`.
* **Example Configurations & Documentation (`example/`, `docs/`, `cmake/`, `design/`):**
  * Remove `"max-num-events": 200` from `example/config/server.json`, `example/monitor/config/server.json`, `example/secure/config/signed_server.json`, `example/secure/config/signed_tls_server.json`, and `example/systemd/server.json`.
  * Remove `"max-num-events"` from `docs/apps/apx_server.rst`, `cmake/README.md`, and `design/multiple_providers_policy_spec.md`.

## 18. Improved Parser Error Diagnostics (Column & Character Offsets)

* **Background & Motivation:**
  * When parsing APX definitions (`.apx`), syntax errors in port declarations, type declarations, attributes, or data signatures currently only report the line number (`apx_parser_get_error_line()`).
  * In complex nested record signatures or large definitions, developers must manually hunt through long lines to find syntax errors (e.g., missing closing braces, invalid type codes, stray characters).
  * `apx_signature_parser_t` already records the exact failure pointer (`error_pos`) internally via `apx_signature_parser_set_error()`, but `apx_parser_t` currently passes `NULL` and does not compute or expose column/byte offsets.

* **Parser State & API Enhancements (`apx_parser`):**
  * Add `int32_t error_column` (1-based column offset) and `int32_t error_offset` (byte offset) to `apx_parser_t`.
  * Add accessor: `int32_t apx_parser_get_error_column(const apx_parser_t *self);`.
  * Update `state_parse_data_signature()` in `parser.c` to pass `&error_pos` into `apx_signature_parser_get_last_error(parser, &error_pos)` and compute column offset relative to `line_begin`.
  * Update `attribute_parser` and `parser_base` to record error positions for attribute syntax errors, stray characters, and unmatched delimiters.

* **Diagnostic Formatting & Server Logging:**
  * Implement compiler-style diagnostic formatting:
    `"<file>:<line>:<col>: error: <description>\n    <line_text>\n    ^\n"`
  * Integrate into `apx_server` connection admission logging when rejecting invalid node definitions, making misconfigured client definitions immediately identifiable in logs.

* **Test Coverage (`testsuite_parser.c`):**
  * Unit tests verifying exact line and column numbers for:
    * Unmatched bracket/brace inside complex record signatures.
    * Invalid type specifier characters.
    * Exceeding `APX_MAX_PSG_LEN`.
    * Malformed attribute syntax and stray characters.

## 19. Handshake & Greeting Timeout (Slowloris / Handshake Starvation Protection)

* **Background & Motivation:**
  * When a client connects via TCP, UNIX domain socket, or VSOCK, `apx_server_accept_connection()` immediately accepts the connection and attaches it to `connection_manager->active_connections`, consuming 1 slot toward `max-concurrent-connections`.
  * The connection enters `APX_CONNECTION_STATE_CONNECTING` awaiting the RemoteFile greeting header (`RMF_GREETING_1_1_START` / `RMF_GREETING_1_0_START`).
  * If an unauthenticated or malicious client connects and remains completely silent (or sends incomplete bytes), the server never times out the connection.
  * Spawning `max-concurrent-connections` (default: 256) silent dummy connections exhausts all available connection slots, causing legitimate APX client nodes to be rejected indefinitely with `"Concurrent connection limit exceeded"`. Even if cryptographic signature verification (`require-signed-nodes`) is enabled, it is never reached because authentication requires the greeting header and definition transmission.

* **Connection Timestamp & State Tracking (`server_connection`):**
  * Record `uint64_t connect_monotonic_time_ms` (or `connect_monotonic_time_ns`) on `apx_server_connection_t` upon creation using `cutil_get_monotonic_time_ms()` / `apx_get_monotonic_time_ns()`.
  * Track whether the handshake has been completed (`APX_CONNECTION_STATE_ACCEPTED`).

* **Background Timeout Audit in Connection Manager (`connection_manager.c`):**
  * Enhance `cleanup_task` in `apx_connection_manager_t` to audit `active_connections` during its periodic cycle.
  * For any connection in `active_connections` whose state is not `APX_CONNECTION_STATE_ACCEPTED` and whose elapsed time exceeds `handshake_timeout_ms`:
    * Log a warning (`"[SERVER] Handshake timeout: closing inactive connection %u"`).
    * Forcibly close the transport socket (`apx_connection_base_close(&conn->base)`).
    * Detach the connection and move it to `inactive_connections` for cleanup, immediately freeing the connection slot for legitimate nodes.

* **Safe Connection Teardown & Event Synchronization (`connection_manager.c`):**
  * In `apx_connection_manager_cleanup_task_main()`, flush pending server events via `apx_server_flush_events(server)`
    prior to invoking `apx_connection_base_delete(&serverConnection->base)`.
  * Guarantees that any in-flight events referencing the connection (e.g. `APX_EVENT_PROTOCOL_HEADER_ACCEPTED` or
    `APX_EVENT_REMOTE_FILE_PUBLISHED`) are fully processed by the event loop thread before the connection memory and
    its internal mutexes are destroyed, preventing data races during rapid client disconnects.

* **Server JSON Configuration (`server.json` / `server_cfg.c`):**
  * Support `"handshake-timeout-ms"` (with alias `"handshake_timeout_ms"`, default: `3000` ms; `0` to disable) under the `"apx-server"` section of `server.json`.
  * Validate that the value is a positive integer (or `0`); emit a warning on invalid input and default to `3000`.
  * Expose getter/setter on `apx_server_t`: `apx_server_set_handshake_timeout_ms()` / `apx_server_get_handshake_timeout_ms()`.

* **Test Coverage:**
  * Unit tests in `testsuite_server.c` / `testsuite_connection_manager.c` verifying that connections that do not send a greeting header within the configured timeout are evicted and their slots reclaimed.
  * Unit tests in `testsuite_server_cfg.c` for parsing `"handshake-timeout-ms"` and invalid-value warnings.
  * Integration tests in `tests/test_server_lifecycle.py` simulating silent/unresponsive clients and verifying that legitimate clients can connect after timeouts expire.

## 20. Compression Backend and CMake Integration

**Status:** Completed. Provide the codec and build infrastructure required by [item 16](#16-compressed-transfer-of-apx-node-definitions-signed-and-unsigned), independently of RemoteFile, signing, or caching.

* **Internal API:** Support both compression and bounded decompression of one zlib-wrapped DEFLATE stream (RFC 1950 / RFC 1951), without preset dictionaries. Dynamic C clients need compression; servers receiving definitions need decompression. Define explicit buffer ownership, output sizes, and repository-standard error reporting. Keep library-specific includes, calls, and preprocessor selection inside the compression module.
* **Backend Selection:** Add one CMake cache string, `APX_COMPRESSION_BACKEND`, accepting `zlib` or `miniz`. Default to zlib for Linux targets and miniz for Windows targets, with explicit overrides on either platform. Compile and link exactly one backend; reject invalid selections or unavailable dependencies rather than silently switching libraries. Keep backend compile definitions target-private.
* **Dependency Acquisition:** Use `find_package(ZLIB REQUIRED)` and `ZLIB::ZLIB` for zlib. Prefer an installed miniz CMake package; otherwise use `FetchContent` with a pinned upstream `richgel999/miniz` release archive and verified SHA-256. Build with the selected toolchain and disable unrelated dependency examples/tests. Do not introduce a Git submodule or require vcpkg.
* **Offline and Reproducible Builds:** Support installed packages and CMake's `FETCHCONTENT_SOURCE_DIR_MINIZ` override without network access. Document how to disable automatic fetching and fail clearly when dependencies are unavailable. Keep downloads in the build tree and report the selected backend/version during configuration.
* **Consistent Codec Behavior:** Enforce output limits during decompression, exact input consumption, complete stream termination, valid zlib headers/checksums, and rejection of preset dictionaries, trailing/concatenated data, raw DEFLATE, and gzip. Handle allocation failures and malformed input explicitly, release all resources, and support concurrent independent calls without shared mutable codec state.
* **Documentation and Tests:** Document backend selection, platform defaults, dependency/version/license handling, and offline setup. Exercise the same codec tests under both backends, including binary data with embedded zero bytes, empty input, insufficient output space, malformed/truncated streams, allocation/error cleanup, cross-backend interoperability, and Python zlib interoperability. Add focused build/test configurations covering both choices and run the existing ASan/UBSan and mandatory TSan checks for the implementation.

**Acceptance criteria:** Linux can build and pass the codec tests with either backend; Windows defaults to miniz and supports explicit selection of zlib when supplied. Available local dependencies require no download. Both backends produce interoperable streams and equivalent validation/error behavior, without requiring identical compressed bytes.

**Out of scope:** RemoteFile publication and opcode changes, transfer/signature/cache integration (item 16), runtime backend switching, a general-purpose compression framework, and changes to py-apx or APX-ES. Precompressed APX-ES nodes need no runtime codec; other-language clients use their own compatible implementations.

The backend does not imply compression of `server.log` or `.apxlog` containers.
Logical-definition capture and structured rejection events are owned by item 16.

## 21. Retire Legacy `cutil` SHA-256 in Favor of Mbed TLS (`apx_crypto_sha256`)

* **Status:** Proposed. Eliminate redundant SHA-256 implementations and unify cryptographic hashing across `c-apx`.

* **Background & Motivation:**
  * Historically, a public-domain implementation of SHA-256 was bundled into the `cutil` submodule (`cutil/include/sha256.h`, `cutil/src/sha256.c`).
  * With the integration of Mbed TLS for node authentication, TLS transports, and digital signatures, `apx_core` introduced `apx_crypto_sha256()` in [`apx_core/src/crypto.c`](apx_core/src/crypto.c), which calls `mbedtls_sha256()`.
  * Currently, the codebase has mixed usage:
    * [`apx_core/src/node_instance.c`](apx_core/src/node_instance.c) and several connection test suites (`testsuite_client_socket_connection.c`, `testsuite_client_test_connection.c`, etc.) still `#include "sha256.h"` and invoke `sha256_calc()`.
    * [`apx_core/src/node_manager.c`](apx_core/src/node_manager.c), [`apx_core/src/crypto.c`](apx_core/src/crypto.c), and [`app/apx_sign`](app/apx_sign) use `apx_crypto_sha256()` / Mbed TLS.
  * Maintaining two independent SHA-256 implementations causes binary bloat, inconsistent APIs, and confusion regarding which hashing function should be used for file digests, caching, and cryptographic verification.

* **Core Tasks (`apx_core`):**
  * Replace all occurrences of `#include "sha256.h"` and `sha256_calc(...)` in [`apx_core/src/node_instance.c`](apx_core/src/node_instance.c) with `#include "apx/crypto.h"` and `apx_crypto_sha256(...)`.
  * Update C unit test suites in `apx_core/test/` (`testsuite_client_socket_connection.c`, `testsuite_client_socket_monitor_connection.c`, `testsuite_client_test_connection.c`, `testsuite_server_connection.c`) to use `apx_crypto_sha256()`.
  * If multi-chunk/streaming hashing is required for large file streams in future items, expose streaming wrapper functions in `apx/crypto.h` (`apx_crypto_sha256_init`, `apx_crypto_sha256_update`, `apx_crypto_sha256_finish`) wrapping `mbedtls_sha256_starts`, `mbedtls_sha256_update`, `mbedtls_sha256_finish`.

* **Submodule & Build Cleanup (`cutil`):**
  * Remove `sha256.h` dependency from `c-apx` include search paths if feasible.
  * In the `cutil` upstream repository, deprecate/retire `sha256.c` and `sha256.h` once no consumers remain.

* **Acceptance criteria:**
  * Zero references to `sha256.h` or `sha256_calc()` in `c-apx` source and test files.
  * All definition hashing, signature verification, and test fixtures consistently use `apx_crypto_sha256()`.
  * All unit and integration test suites pass with zero memory leaks and zero warnings under ASan and TSan.

---

## 22. Investigate Unicode (Non-ASCII) Characters in APX Definitions

* **Status:** Proposed investigation; no current parser vulnerability has been established.
* Check how the APX definition parser handles Unicode encoded as UTF-8 in node, port, and type names, quoted strings, and other definition fields. Compare accepted and rejected input with the normative APX IDL specifications before deciding on the intended behavior.
* Test non-ASCII characters at the beginning and within lines, malformed UTF-8, and multibyte sequences split across input chunks.
* Verify that unsupported or malformed input produces an explicit parsing error without assertion failures, infinite loops, or loss of server availability, and that incomplete input is buffered safely.
* Add regression coverage and document the supported encoding and any field-specific restrictions.

---

# Release v0.8.0 (Future Phase)

## 1. Event Stream Playback Engine (`apx-log replay`)

* Implemented during the v0.7.x phase as companion tooling for GUI log viewers and analyzers.
* Detailed requirements defined in [event_streaming_and_apxlog_spec.md](design/event_streaming_and_apxlog_spec.md) (Section 11) and [diagnostics_and_logging_architecture.md](design/diagnostics_and_logging_architecture.md) (Section 2.2).
* **Playback Functionality:**
  * Connects to the operational APX endpoint (port `5000` / standard UNIX socket) as a standard multi-node client.
  * Reconstructs initial node definitions and data files from the `.apxlog` snapshot (`STREAM_BEGIN` through `SNAPSHOT_END`).
  * Replays recorded `.out` signal writes based on monotonic `timestamp_ns` timing deltas:
    `target = playback_start + (event_time - first_replayed_time) / rate`
  * Playback controls:
    * `--rate <float>`: Time scaling (e.g. `1.0` real-time, `2.0` 2x speed, `0.5` half speed).
    * `--loop`: Loop playback indefinitely.
    * `--exclude-node <name>` / `--include-nodes <names>`: Node filtering to simulate surrounding vehicle bus nodes while running real ECU node under test.
