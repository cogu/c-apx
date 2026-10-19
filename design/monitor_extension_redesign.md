# Proposal for a Cleaned-Up `apx_monitor_extension`

## 1. Overview & Motivation

The original 2020/2021 prototype of `apx_monitor_extension` relied on:
* Sharing the operational port (`5000`) with standard node connections and switching into `APX_CONNECTION_TYPE_MONITOR` after parsing the greeting header.
* Custom low-level RemoteFile command opcodes (`RMF_CMD_CONNECTION_CREATE`, `RMF_CMD_PUBLISH_REMOTE_FILE_MSG`).
* Maintaining continuous shadow copies of server state (`apx_observed_connection_t`, `apx_observed_file_t`, and `rmf_extended_file_info_t`) on every connection, file, and write event—even when no monitor client was connected.

With the introduction of RemoteFile v1.1 Stream Files (`FileStream`), dedicated diagnostic endpoints (`5080` / `/run/apx/apx_monitor.socket`), and the companion specifications ([`diagnostics_and_logging_architecture.md`](diagnostics_and_logging_architecture.md), [`apxmon_diagnostics_spec.md`](apxmon_diagnostics_spec.md), [`event_streaming_and_apxlog_spec.md`](event_streaming_and_apxlog_spec.md), and [`securing_monitor_extension.md`](securing_monitor_extension.md)), the extension should be rewritten to support both **`apxmon`** (interactive diagnostics) and **`apxlog`** (event streaming and recording) from a single, unified server extension.

---

## 2. File Structure

```text
extension/apx_monitor_extension/
├── include/apx/extension/
│   ├── monitor_extension.h   # Public registration entry point for apx-server
│   ├── server_monitor.h      # Owns msocket_server_t (TCP 5080 / UNIX / mTLS) & server hooks
│   ├── monitor_session.h     # Per-client session (manages the 3 FileStream channels)
│   ├── diag_dispatcher.h     # Handles apx.diag.v1.request -> response & notify (apxmon)
│   └── log_streamer.h        # Handles server.log snapshot + lock-free ring buffer (apxlog)
└── src/
    ├── monitor_extension.c
    ├── server_monitor.c
    ├── monitor_session.c
    ├── diag_dispatcher.c
    └── log_streamer.c
```

### Module Responsibilities

| Module | Responsibility |
| :--- | :--- |
| **`monitor_extension`** | Parses `"monitor-extension"` configuration from `server.json` and registers `init` and `shutdown` lifecycle callbacks with `apx_server_t`. |
| **`server_monitor`** | Owns the dedicated `msocket_server_t` listener (UNIX domain socket and TCP/mTLS on port `5080`), tracks active `apx_monitor_session_t` connections, and registers the fast-path gated event listener on `apx_server_t`. |
| **`monitor_session`** | Created when a diagnostic or logging client connects on port `5080`. Wraps `apx_server_connection_t`, publishes `server.log`, `apx.diag.v1.response`, and `apx.diag.v1.notify` (`FileStream`), and routes stream open/close/write callbacks to either `diag_dispatcher` or `log_streamer`. |
| **`diag_dispatcher`** | Decodes `apxmon` requests from `apx.diag.v1.request`, takes brief read-only snapshots from `apx_server_t` for `LIST_*` and `GET_NODE_DEFINITION`, manages `SUBSCRIBE_PORTS` watches, and writes responses to `apx.diag.v1.response` and notifications to `apx.diag.v1.notify`. |
| **`log_streamer`** | Activated only when a session opens `server.log`. Generates the initial `STREAM_BEGIN` $\to$ `SNAPSHOT` $\to$ `SNAPSHOT_END` sequence and drains the lock-free event ring buffer into `server.log` while respecting `FileStream` window credits. |

---

## 3. Legacy Components to Retire

The following legacy files and types are superseded by this design and should be removed:

| Component | Location | Reason for Removal |
| :--- | :--- | :--- |
| `apx_observed_connection_t` | `extension/apx_monitor_extension/` | Replaced by on-demand, read-only snapshots taken directly from `apx_server_t` when queried. |
| `apx_observed_file_t` | `extension/apx_monitor_extension/` | Only tracked RemoteFile names/sizes (`FileFixed`), which cannot answer node, port, or routing queries and incurred lock/search overhead on every signal write. |
| `rmf_extended_file_info_t` | `apx_core/` | Duplicated `rmf_file_info_t` fields and implemented obsolete wire opcodes (`24u`/`25u`) that are now used by RemoteFile v1.1 (`RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE` and `RMF_CMD_STREAM_OPEN`). |
| `client_monitor_connection.*` | `extension/apx_monitor_extension/` | Empty 2021 stub; client connection classes do not belong inside a server extension. |
| `monitor_socket_client_connection.*` | `extension/apx_monitor_extension/` | Outdated duplicate of `apx_client_socket_connection_t` in `apx_core` (which already supports `APX_CONNECTION_TYPE_MONITOR`, TLS, and UNIX sockets). |

---

## 4. Serving Both `apxmon` and `apxlog` from One Extension

A single `apx_monitor_extension` cleanly supports both interactive query/response (`apxmon`) and continuous event recording (`apxlog`) without mode switching:

1. **Shared Listener & Security Boundary**:
   * Both tools connect to the same privileged diagnostic endpoint (TCP port `5080` with mTLS or `/run/apx/apx_monitor.socket`).
2. **Intent-Based Stream Multiplexing**:
   * Upon completing the RMFP 1.1 greeting, every `apx_monitor_session_t` publishes three local `FileStream` files:
     * `server.log`
     * `apx.diag.v1.response`
     * `apx.diag.v1.notify`
   * The connected client declares its role purely through standard RemoteFile operations:
     * **`apxlog record`** sends `RMF_CMD_STREAM_OPEN` for `server.log`, activating `log_streamer` for that session.
     * **`apxmon`** publishes `apx.diag.v1.request` and sends `RMF_CMD_STREAM_OPEN` for `apx.diag.v1.response` (and `apx.diag.v1.notify` when using `apxmon watch`), activating `diag_dispatcher` for that session.
3. **Zero Overhead When Idle**:
   * `apx_server_t` remains the single source of truth.
   * Paged `LIST_*` queries take a quick, lock-protected snapshot of `apx_server_t` only when requested (`snapshot_id == 0`) and immediately release the server lock before encoding or transmitting responses.
   * Server event hooks check atomic subscriber counters (`num_active_log_streams` and `num_active_port_watches`) and return immediately when no client is actively recording or watching ports.
