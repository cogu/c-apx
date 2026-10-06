# c-apx Roadmap

These are the items left to implement for c-apx v0.4.0.

## 1. Analyze all TODO comments in the c-apx code (Completed)

* [x] **Completed**: All inline TODO comments across the repository have been investigated and resolved:
  * Implemented missing functionality (e.g. `apx_vm_deserializer_unpack_char8`, file manager error handling/cleanup, socket accept cleanup).
  * Removed obsolete, stale, and duplicate comments.
  * Extracted major architectural features into dedicated items in this roadmap (Items 7, 11, 12, 13).

## 2. Design and Implement Event Streaming Protocol & Binary Log Format

* **Application-Level Event Streaming Protocol (RMFP 1.1 Command Area):**
  * Define `RMF_CMD_EVENT_LOG` / `RMF_CMD_EVENT_STREAM` opcode with a structured header (`timestamp_ns`, `event_type`, `source_id`, `payload_len`).
  * Implement encoder and decoder functions in `remotefile.c`/`remotefile.h`.
* **High-Precision Cross-Platform Timestamps:**
  * Implement portable `apx_get_timestamp_ns()` in `cutil` / `osutil.h` using `GetSystemTimePreciseAsFileTime` (Windows) and `clock_gettime(CLOCK_REALTIME)` (Linux/POSIX) returning nanoseconds since Unix Epoch (1970-01-01).
* **Binary Log File Format (`.apxrec` / `.apxlog`):**
  * Design chunked, append-only binary container format (file magic header + metadata + sequential event records matching the network payload).
  * Implement file writer/recorder and file reader/playback modules for recording and replaying APX event streams.

## 3. Complete File Observation in Server Monitor

* Complete `apx/extension/src/observed_file.c` & `observed_file.h`:
  * Implement `apx_observedFile_create()` to store `rmf_extendedFileInfo_t` / file properties.
  * Fix syntax typo in `apx_observedFile_delete()` (stray semicolon on `if (self != NULL);`).
* In `apx/extension/src/server_monitor.c`:
  * Register `file_published` and `file_revoked` listeners in `register_connection_listener()`.
  * Track published files per connection in `apx_observedConnection_t->file_list`.

## 4. Live Event Broadcast to Connected Monitors

* In `apx/extension/src/server_monitor.c`:
  * When a new regular connection is accepted/established after a monitor is already active, broadcast `RMF_CMD_CONNECTION_CREATE` to all active `monitor_connections`.
  * When a connection disconnects, broadcast `RMF_CMD_CONNECTION_REVOKE` to all active `monitor_connections`.
  * Broadcast file publish/revoke notifications to active monitor connections.

## 5. Complete `apx_info` CLI Application

* In `app/apx_info/src/apx_info_main.c`:
  * Add reception and decoding of incoming `RMF_CMD_CONNECTION_CREATE` / `RMF_CMD_CONNECTION_REVOKE` packets.
  * Implement table/formatted printing of connected clients and nodes for the `clients` command.
  * Handle clean disconnect and exit once initial report or interactive monitor loop is completed.
* Resolve/reconcile client monitor abstractions (`client_monitor_connection.c` / `monitor_socket_client_connection.c`).

## 6. Dedicated Diagnostic Transport & TLS Security for Server Monitor

* Detailed architecture and design defined in [securing_monitor_extension.md](securing_monitor_extension.md).
* **Dedicated Listener Integration (`apx_monitor_extension`):**
  * Decouple monitor connections from `apx_socket_extension` so the monitor extension manages its own dedicated `msocket_server_t`.
  * Support listening on a dedicated UNIX domain socket (e.g. `/run/apx/apx_monitor.socket`) and/or dedicated TCP port (e.g. 5002) configured via `server.json`.
  * Automatically instantiate accepted connections directly as `APX_CONNECTION_TYPE_MONITOR`.
* **Mbed TLS Transport Security (mTLS / PSK):**
  * Integrate `msocket_tls_config_t` into `apx_server_monitor_t` to support TLS on the diagnostic TCP port.
  * Enforce Mutual TLS (`require-client-cert: true`) so only authorized diagnostic clients possessing a valid client certificate can connect.
  * Verify client certificate Subject / Common Name to prevent standard node certificates from connecting to the monitor endpoint.
* **Update `apx_info` CLI Application:**
  * Add TLS connection arguments (`--tls`, `--ca-cert`, `--client-cert`, `--client-key`) and default port/socket to target the dedicated monitor endpoint.

## 7. Implement Worker Command Stubs (`file_manager_worker.c`)

* Implement `APX_CMD_REVOKE_LOCAL_FILE` (line 351).
* Implement `APX_CMD_CLOSE_REMOTE_FILE` (line 357).
* [x] Implement `APX_CMD_SEND_ERROR_CODE` (completed).

## 8. Shared Library Symbol Visibility and Windows DLL Export

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

## 9. Support Newline-Delimited JSON (NDJSON) in `apx-node` JSON Message Server

Currently, `app/apx_node/src/json_server_connection.c` expects every message to be framed using a binary `numheader32` length prefix. This prevents generic tools (such as `nc`, `socat`, or standard shell pipelines) from sending raw JSON directly (e.g. `echo '{"VehicleSpeed": 72.5}' | nc -U /tmp/apx_node.socket` fails because `{` / `0x7B` is interpreted as a binary length byte).

* **Auto-Detection / Multi-Framing Support:**
  * In `json_server_connection_data()`: Inspect the initial byte of the incoming buffer.
  * If the first byte is `{` or whitespace (`0x20`, `\t`, `\r`, `\n`), parse the stream using newline delimitation (`\n` / `\r\n`) or balanced-brace boundaries.
  * If the first byte is a binary `numheader` byte, preserve the existing `numheader_decode32()` framing for backward compatibility with `apx_control`.
* **Testing & Tooling:**
  * Enable direct shell integration and piping from `nc -U`, `socat`, and standard POSIX utilities without needing helper wrappers.
  * Add pytest integration test verifying NDJSON updates sent via raw socket.

## 10. Modernize TextLog Extension & Logging Infrastructure

The `c-apx` codebase contains numerous compile-time `#if APX_DEBUG_ENABLE` checks and raw `printf` statements across server and connection components. While `apx_log_level_t` exists in `apx_core/include/apx/types.h` and `apx_server_log_write()` is implemented, the `apx_text_log_extension` ignores log levels and lacks runtime level filtering.

* [x] **TextLog Extension Configuration & Level Filtering:**
  * Support a `"log-level"` setting (values: `"CRITICAL"`, `"ERROR"`, `"WARNING"`, `"INFO"`, `"DEBUG"`; default: `"INFO"`) in `server.json` under `"textlog-extension"`.
  * Add string conversion utilities (`apx_log_level_from_string()` / `apx_log_level_to_string()`).
  * Add `log_level` to `apx_text_log_base_t` / `apx_server_text_log_t` and filter incoming log events so only messages with `level <= configured_level` are output.
* [x] **Standardized Log Message Formatting:**
  * Support a `"use-timestamp"` boolean setting (default: `false`) in `server.json` under `"textlog-extension"`.
  * Format output with consistent prefixes: `[<timestamp>] [<LEVEL>] [<LABEL>] <message>` (or `[<LEVEL>] [<LABEL>] <message>` when timestamps are disabled).
  * Classify built-in server events in `server_text_log.c` with appropriate log levels instead of direct unlevelled prints (e.g., connections/disconnections and file publishes as `INFO`, port routing and protocol header handshakes as `DEBUG`).
* [x] **Replace Legacy Server `APX_DEBUG_ENABLE` Prints:**
  * Replace compile-time `#if APX_DEBUG_ENABLE` prints in server modules (`server_connection.c`, `connection_manager.c`, `socket_server_connection.c`, `socket_server.c`, `tls_server.c`, `tls_server_connection.c`) with `apx_server_log_write(server, APX_LOG_LEVEL_DEBUG, ...)`.
  * Ensure high-frequency debug logging (such as packet reception/transmission) does not exhaust server event loop memory via early check on active log listeners before event allocation.
* [ ] **Complete Syslog Integration (`syslog-enabled`):** *(Deferred to separate commit)*
  * Wire up the existing `syslog-enabled` placeholder in `server_text_log_extension.c` and `text_log_base.c` using POSIX `syslog(priority, ...)`, mapping `apx_log_level_t` directly to syslog priorities (`LOG_CRIT`, `LOG_ERR`, `LOG_WARNING`, `LOG_INFO`, `LOG_DEBUG`).

## 11. Small Data Support in Command Pipeline

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

## 12. Support Multiple Providers per Require Port

In `apx_node_instance_handle_require_ports_disconnected` (`apx_core/src/node_instance.c`), the connector change table supports disconnect transitions where `entry->count == -1` (single provider disconnection). When `entry->count < -1`, the function currently returns `APX_NOT_IMPLEMENTED_ERROR`.

In `apx_port_connector_change_entry_t` (`apx_core/include/apx/port_connector_change_entry.h`), `union portref_union_tag` already provides an `adt_ary_t *array` when `count < -1` or `count > 1`, designed to hold multiple provider references.

* **Multi-Provider Disconnect Handling (`node_instance.c`):**
  * When `entry->count < -1`, iterate over all disconnected provide ports stored in `entry->data.array` using `apx_port_connector_change_entry_get(entry, i)`.
  * For each provide port, acquire `apx_node_instance_lock_port_connector_table(provide_node_instance)`, invoke `remove_provide_port_connector()`, compute the updated provide port count, and unlock.
  * Send provide port count data notifications for all affected provide node instances.
  * Send require port count update for the require port.
* **Testing:**
  * Add unit and integration tests verifying multi-provider connect and simultaneous disconnect transitions.

## 13. Node Definition Cache (L1 In-Memory & L2 Persistent Disk Cache)

Currently in `apx_node_manager_remote_file_published_notification` (`apx_core/src/node_manager.c`), `*file_open_request` is unconditionally set to `true`. This causes the server to request and transfer the `.apx` definition file over the network for every connecting node, even when the remote node provides a definition checksum (`rmf_file_info_digest_data()`) identical to a known or previously parsed definition.

In `server_connection.c`, bypassing file download when `file_open_request == false` returns `APX_NOT_IMPLEMENTED_ERROR` ("Retrieving data from cache not yet implemented"). `apx_node_cache_t` exists as an initial stub in `apx_core/include/apx/node_cache.h` and `src/node_cache.c`.

* **Level 1 (In-Memory) Cache:**
  * Store parsed node definitions (`apx_node_t`) in a server-wide hash table indexed by node name and SHA-256 / SHA-1 checksum.
  * If a connecting client advertises a matching checksum, attach the existing cached node definition directly.
* **Level 2 (Persistent Disk) Cache:**
  * Store raw `.apx` definition files on disk in a cache directory (e.g. `/var/cache/apx/nodes/` or configurable via `server.json`).
  * On server restart, preload or on-demand load cached definitions from disk when matching checksums are received.
* **Protocol & Connection Handshake:**
  * In `node_manager.c`: if the definition is found in cache, set `*file_open_request = false`.
  * In `server_connection.c`: if `file_open_request` is false, load definition AST directly from cache into `node_instance`, transition data state to `APX_DATA_STATE_READY`, and proceed without requesting the remote `.apx` virtual file.




