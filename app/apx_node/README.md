# apx-node

## Synopsis

```text
apx-node   [-b --bind bind_path] [-p --bind-port port] [--no-bind]
           [-c --connect connect_path] [-r --connect-port connect_port]
           [--no-signature]
           [--tls] [--ca-cert ca_path] [--client-cert cert_path] [--client-key key_path]
           [--vsock <cid>:<port>]
           [--version] [--help]
           file
```

## Description

`apx-node` starts a console application that takes the following initial actions:

- Creates a dynamic APX client based on the definition given by the *file* argument (`.apx`).
- Automatically searches for and loads companion cryptographic signature files (`.apx.sig` or `.sig`).
- Creates a JSON message router socket server (UNIX domain socket or TCP) based on the *bind* options given, unless `--no-bind` is specified.
- Connects to an APX server based on the *connect* options given (over UNIX socket, plain TCP, encrypted TLS, or Linux VSOCK).

Once these preparatory steps have completed, it provides the following runtime functionality:

- **JSON Signal Control**: Listens for JSON messages on the bound socket. Connected external clients can use this socket to inspect and set provide-port values of the node.
- **APX Server Communication**: Exchanging port data over the Remote File Protocol (RMFP) with `apx-server`.
- **Require-Port Monitoring**: When any require-port of the APX node changes value, the new signal value is formatted as JSON and printed to `stdout`.

## Mandatory Arguments

```text
file     Path to an APX definition file (.apx)
```

## Options

```text
-b --bind address_or_path
                Either TCP address or UNIX domain socket path to bind for
                receiving provide-port data updates via JSON.

-p --bind-port port
                Port number for JSON message server socket (TCP only; not
                applicable when using UNIX domain sockets).

--no-bind
                Do not start the JSON message server. Runs the node purely
                as an APX client and signal observer.

-c --connect address_or_path
                Either TCP address/hostname or path to UNIX domain socket to
                connect to the APX server daemon.

-r --connect-port connect_port
                Port number for APX server socket (defaults to 5000 for TCP,
                or 5020 when --tls is specified).

--tls
                Connect to the APX server using encrypted TLS stream transport.
                Defaults to 127.0.0.1:5020 unless -c or -r is overridden.

--ca-cert path
                Path to trusted Root CA certificate PEM file to verify server
                authenticity. Automatically discovers example/secure/config/certs/ca_cert.pem
                or tests/certs/ca_cert.pem if omitted.

--client-cert path
                Path to client certificate PEM file for Mutual TLS (mTLS)
                authentication with the APX server.

--client-key path
                Path to client private key PEM file for Mutual TLS (mTLS)
                authentication with the APX server.

--no-signature
                Ignore companion signature file (.sig) even if one exists in the
                same directory, and force publishing as an unsigned node (RMFP/1.0).

--vsock <cid>:<port>
                Connect to the APX server using Linux VSOCK (AF_VSOCK) transport.
                The cid can be specified as a numeric Context ID (e.g. 2 for host,
                3 for guest) or using symbolic aliases: host, local, hypervisor, or any.
                (Linux only; not supported on Windows).

--version
                Print version information and exit.

-h --help
                Print help message and exit.
```

## Cryptographic Signature Support

`apx-node` supports secure signal publishing through cryptographic authentication (RMFP/1.1):

1. **Automatic Discovery**: When given `file.apx`, `apx-node` automatically checks for a companion signature in the same directory:
   - `<file.apx>.sig`
   - `<file>.sig`
2. **Seamless Elevation**: If a valid 64-byte ECDSA NIST P-256 signature is detected, `apx-node` automatically:
   - Attaches the signature to the definition file metadata.
   - Negotiates protocol version `RMFP/1.1` with the server.
   - Publishes the definition using `RMF_CMD_PUBLISH_SIGNED_FILE_MSG`.
3. **Server Verification**: An `apx-server` configured with `require-signed-nodes: true` will verify the signature against its trusted public keys before accepting the node.
4. **Override (`--no-signature`)**: If you need to run the node in legacy unsigned mode (e.g. against an older `apx-server` or during development), specify `--no-signature` to bypass auto-discovery.

Signature files can be generated using the [`apx-sign`](../apx_sign/README.md) CLI utility.

## Option Default Values

### Linux Defaults

```text
--bind          /tmp/apx_node.socket
--bind-port     5100
--connect       /tmp/apx.socket
--connect-port  5000
```

### Windows Defaults

```text
--bind          127.0.0.1
--bind-port     5100
--connect       127.0.0.1
--connect-port  5000
```

## Example Usage

### Standard Usage

Connect to default local UNIX socket server:

```bash
apx-node vehicle.apx
```

### Connect to Custom APX Server Socket

```bash
apx-node -c /tmp/my_apx.socket vehicle.apx
```

### Connect over TCP/IP

```bash
apx-node -c 192.168.1.19 -r 5000 -p 5101 vehicle.apx
```

### Run as Observer / Listener (No JSON Router)

```bash
apx-node --no-bind -c /tmp/apx.socket listener.apx
```

### Ignore Existing Signature File

```bash
apx-node --no-signature vehicle.apx
```

### Connect over TLS

Connect to APX server using TLS (auto-discovers CA certificate and defaults to port 5020):

```bash
apx-node --tls vehicle.apx
```

Explicit server endpoint and CA certificate:

```bash
apx-node --tls --ca-cert example/secure/config/certs/ca_cert.pem -c 127.0.0.1:5020 vehicle.apx
```

### Connect over Mutual TLS (mTLS)

Authenticate client node identity using client certificate and key:

```bash
apx-node --tls --ca-cert ca_cert.pem --client-cert client_cert.pem --client-key client_key.pem vehicle.apx
```

### Connect over VSOCK

Connect to an APX server on the hypervisor/host (CID 2, port 5000):

```bash
apx-node --vsock 2:5000 vehicle.apx
```

Or using the symbolic alias `host`:

```bash
apx-node --vsock host:5000 vehicle.apx
```

Connect to an APX server running locally in loopback (requires `vsock_loopback` driver):

```bash
apx-node --vsock local:5000 vehicle.apx
```
