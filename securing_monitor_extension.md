# Securing the APX Server Monitor Extension

## 1. Overview and Problem Statement

The APX server monitor extension (`apx_monitor_extension`) provides runtime introspection, diagnostics, and telemetry for the APX daemon. Its capabilities include:

* Observing all active client connections and lifecycle state transitions.
* Inspecting and downloading attached virtual `.apx` definition files for all connected nodes.
* Streaming real-time signal modifications and execution events (`RMF_CMD_EVENT_STREAM`).

Because the monitor connection operates as an administrative tap with visibility into all node definitions and bus signals, exposing it without strict access controls and transport segregation introduces severe security and operational risks.

---

## 2. Dedicated Diagnostic Listener vs. In-Band Mode Switching

### Why Dedicated Transport is Preferred
1. **Principle of Least Privilege & Perimeter Defense**:
   * If regular ECU clients and diagnostic monitors share the standard connection port (e.g., TCP 5000), any connected client could attempt to switch into monitor mode and eavesdrop on the entire system.
   * A dedicated port (e.g., TCP 5002) allows granular firewall/iptables rules, network segmentation (e.g., binding to a private diagnostic VLAN or `127.0.0.1`), and independent access control.
2. **Elimination of Dynamic State Mutation**:
   * Dynamically switching an active connection from an APX data client to a monitor mid-flight creates complex lifecycle hazards (such as revoking local files, tearing down port maps, and modifying routing tables).
   * With a dedicated listener, any accepted connection is typed as `APX_CONNECTION_TYPE_MONITOR` from its first byte.
3. **Traffic Isolation & Quality of Service (QoS)**:
   * Streaming real-time event logs and downloading full `.apx` definitions can be bursty and high-bandwidth. Segregating this traffic prevents head-of-line blocking and buffer starvation on deterministic, low-latency signal channels.
4. **Extension Modularity**:
   * Decouples `apx_monitor_extension` from `apx_socket_extension`. The monitor extension manages its own `msocket_server_t` instance. When disabled in configuration (`"enabled": false`), no diagnostic endpoint is created.

---

## 3. Security Architecture (Mbed TLS Integration)

### 3.1 Remote Telemetry: Mutual TLS (mTLS)
Standard server-side TLS encrypts data in transit and authenticates the server to the client, but **does not protect against unauthorized clients connecting to the diagnostic port**.

For remote monitoring over TCP, **Mutual TLS (mTLS / Client Certificate Authentication)** is the required standard:
* **Enforced Verification**: The APX server requires the remote monitor to present an X.509 client certificate signed by an authorized Diagnostic Certificate Authority (CA).
* **Early Rejection**: Handshakes from unauthorized or untrusted clients fail at the transport layer before any application bytes or connection events are exchanged.
* **`msocket` Integration**: Utilizes existing APIs in `msocket_tls.h`:
  ```c
  msocket_tls_config_set_server_cert(&tls_config, server_cert, server_key);
  msocket_tls_config_set_ca_cert(&tls_config, ca_cert);
  msocket_tls_config_set_require_client_cert(&tls_config, true);
  ```

### 3.2 Subject / Role Verification (Defense in Depth)
If the same CA issues certificates across the fleet, the server should verify the client certificate's Subject Common Name (CN), Subject Alternative Name (SAN), or Extended Key Usage (EKU) (e.g., `CN=apx-diag-client`) to ensure general node certificates cannot be used on the monitor endpoint.

### 3.3 Embedded Alternative: TLS-PSK (Pre-Shared Key)
For resource-constrained automotive ECUs lacking a full PKI infrastructure or CRL/OCSP management:
* Mbed TLS supports TLS-PSK (`MBEDTLS_KEY_EXCHANGE_PSK_ENABLED`).
* Offers mutual authentication and authenticated encryption (e.g., AES-GCM) with minimal RAM and flash overhead, completely bypassing X.509 certificate parsing.

### 3.4 Local Diagnostics: Dedicated UNIX Domain Socket
For local inspection tools on the same Linux host (e.g., `apx_info`):
* Bind to a dedicated UNIX domain socket (e.g., `/run/apx/apx_monitor.socket`).
* Restrict access via filesystem permissions (e.g., `chmod 0660`, group `apxdiag`).
* Optional `SO_PEERCRED` check to verify the caller's UID/GID without cryptographic overhead.

---

## 4. Proposed Configuration Schema (`server.json`)

In `server.json`, configure the monitor extension under the `"monitor-extension"` block:

```json
{
  "monitor-extension": {
    "enabled": true,
    "socket-file": "/run/apx/apx_monitor.socket",
    "tcp-port": 5002,
    "bind-address": "127.0.0.1",
    "tls": {
      "server-cert": "/etc/apx/certs/diag_server.crt",
      "server-key": "/etc/apx/certs/diag_server.key",
      "ca-cert": "/etc/apx/certs/diag_ca.crt",
      "require-client-cert": true
    }
  }
}
```

### Configuration Parameters

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `enabled` | Boolean | `true` | Enables or disables the monitor extension. |
| `socket-file` | String | `NULL` | Path to UNIX domain socket for local monitoring. |
| `tcp-port` | Integer | `0` | TCP port for remote monitoring (0 disables TCP listener). |
| `bind-address` | String | `"127.0.0.1"` | IP address to bind TCP listener to (`"0.0.0.0"` for all interfaces). |
| `tls` | Object | `NULL` | TLS configuration object (if omitted, TCP operates plaintext). |
| `tls.server-cert` | String | `NULL` | Path to PEM server certificate. |
| `tls.server-key` | String | `NULL` | Path to PEM private key. |
| `tls.ca-cert` | String | `NULL` | Path to PEM CA certificate for client verification. |
| `tls.require-client-cert` | Boolean | `true` | Enforce mutual TLS (mTLS) client authentication. |

---

## 5. Client Application (`apx_info`) Updates

To communicate with the secured monitor endpoint, `apx_info` will be updated to:
1. Accept `--port 5002` (or default to the diagnostic port/socket).
2. Support optional TLS flags:
   * `--tls`: Enable TLS connection.
   * `--ca-cert <path>`: Server verification certificate.
   * `--client-cert <path>` & `--client-key <path>`: Client identity for mTLS.
3. Connect using `msocket_connect_tls()` with a configured `msocket_tls_config_t`.

---

## 6. Implementation Steps

1. **Listener Integration in `apx_monitor_extension`**:
   * Add `msocket_server_t` instance to `apx_server_monitor_t`.
   * Parse configuration in `apx_monitor_extension_init()` and start UNIX socket / TCP listener.
   * Set up `msocket_tls_config_t` when the `tls` block is present.
2. **Direct Connection Setup**:
   * Incoming connections accepted on this listener are instantiated directly as `APX_CONNECTION_TYPE_MONITOR`.
3. **`apx_info` Client Support**:
   * Add TLS argument parsing and connection support in `app/apx_info/src/apx_info_main.c`.
4. **Integration Tests**:
   * Add pytest test cases validating:
     * Unauthenticated TCP access to diagnostic port is rejected.
     * mTLS with valid client certificate connects and receives connection/file events.
     * Local UNIX domain socket access with correct file permissions.
