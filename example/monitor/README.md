# Monitor Extension Example

This directory contains an example server configuration for running `apx-server` with an enabled and fully configured `monitor-extension`.

## Overview

The `monitor-extension` provides dedicated diagnostic, introspection, and live signal monitoring capabilities within the APX server daemon, powering diagnostic tools such as `apxmon`.

This example demonstrates how to configure:
- A dedicated local UNIX domain socket (`/tmp/apx_monitor.socket`) for local diagnostic access.
- A dedicated TCP listener on port `5080` secured with mutual TLS (mTLS) authentication.

## Directory Structure

```text
example/monitor/
├── README.md
└── config/
    ├── server.json      # Server configuration with enabled monitor-extension
    └── certs/           # ECDSA (NIST P-256) test certificates and keys
        ├── ca_cert.pem          # Test Root CA certificate
        ├── ca_key.pem           # Test Root CA private key
        ├── server_cert.pem      # Diagnostic server certificate
        ├── server_key.pem       # Diagnostic server private key
        ├── client_cert.pem      # Diagnostic client certificate (for mTLS)
        ├── client_key.pem       # Diagnostic client private key (for mTLS)
        └── client_pubkey.pem    # Diagnostic client public key
```

## Running the Example Server

Start the `apx-server` daemon with the monitor configuration:

```bash
apx-server example/monitor/config/server.json
```

When started, the daemon logs its active listener endpoints:
```text
[INFO ] [MONITOR] Listening on UNIX socket /tmp/apx_monitor.socket
[INFO ] [MONITOR] Listening on TLS TCP port 5080
[INFO ] [SOCKET] Listening on TCP port 5000
[INFO ] [SOCKET] Listening on UNIX socket /tmp/apx.socket
```

## Connecting with `apxmon`

### 1. Via UNIX Domain Socket (Local Access)

```bash
apxmon -c /tmp/apx_monitor.socket info
```

### 2. Via TCP with Mutual TLS (mTLS)

```bash
apxmon -c 127.0.0.1:5080 --tls \
  --ca-cert example/monitor/config/certs/ca_cert.pem \
  --client-cert example/monitor/config/certs/client_cert.pem \
  --client-key example/monitor/config/certs/client_key.pem \
  info
```

### 3. Additional Diagnostic Commands

Query connected client nodes:
```bash
apxmon -c /tmp/apx_monitor.socket clients
```

List registered APX nodes:
```bash
apxmon -c /tmp/apx_monitor.socket nodes
```

Inspect active signal routes or unconnected ports:
```bash
apxmon -c /tmp/apx_monitor.socket routing
apxmon -c /tmp/apx_monitor.socket routing --unconnected
```

Live dashboard:
```bash
apxmon -c /tmp/apx_monitor.socket top
```
