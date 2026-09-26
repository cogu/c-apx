# apx-sign

## Synopsis

```text
apx-sign [command] [options] <file.apx>
```

## Description

`apx-sign` is a standalone cryptographic signing and verification utility for APX node definition files (`.apx`). It conforms to the APX Remote File Protocol (RMFP/1.1) specification for signed file exchange.

Key features:
- **ECDSA NIST P-256 (secp256r1) with SHA-256**: Generates and verifies raw 64-byte binary $(r, s)$ signatures.
- **Deterministic Signatures (RFC 6979)**: Powered by Mbed TLS, producing reproducible signatures without external entropy requirements during signing.
- **PEM Key Management**: Generates standard PEM-encoded private keys and SubjectPublicKeyInfo public keys.
- **Companion Signature Files**: Generates `.sig` companion files designed for automatic discovery by `apx-node`.

## Commands

```text
sign               Sign an APX definition file using a private key PEM (default)
verify             Verify an APX definition file against a public key PEM
keygen [prefix]    Generate a new ECDSA P-256 keypair PEM (default prefix: node)
```

## Options

```text
-k, --key <file>       Private key (sign) or public key (verify) PEM file.
-o, --output <file>    Output signature file (sign) or key prefix (keygen).
-s, --sig <file>       Signature file to verify (defaults to <file.apx>.sig or <file>.sig).
-v, --verify           Switch to verify mode.
-g, --keygen           Switch to keygen mode.
-h, --help             Display usage help and exit.
-V, --version          Display version information and exit.
```

## Examples

### 1. Keypair Generation

Generate a new ECDSA NIST P-256 keypair in PEM format:

```bash
# Generates node_key.pem (private) and node_pubkey.pem (public) in current directory
apx-sign keygen

# Generates certs/client_key.pem and certs/client_pubkey.pem
apx-sign keygen certs/client
```

### 2. Signing an APX Definition File

Sign an APX file with a private key to produce a 64-byte binary signature companion file:

```bash
# Produces vehicle.apx.sig in the same directory
apx-sign -k certs/client_key.pem vehicle.apx

# Explicit sign subcommand
apx-sign sign -k certs/client_key.pem vehicle.apx

# Custom output signature filename
apx-sign -k certs/client_key.pem vehicle.apx -o custom.sig
```

### 3. Verifying an APX Definition File

Verify the integrity and authenticity of an APX file against a public key:

```bash
# Automatically finds vehicle.apx.sig (or vehicle.sig) in the same directory
apx-sign verify -k certs/client_pubkey.pem vehicle.apx

# Verify using flag syntax
apx-sign -v -k certs/client_pubkey.pem vehicle.apx

# Verify using an explicit signature file
apx-sign verify -k certs/client_pubkey.pem vehicle.apx -s custom.sig
```

Exit codes:
- `0`: Signature is valid and payload matches the public key.
- `1`: Signature verification failed, file is tampered with, or invalid arguments.

## Integration with `apx-node` and `apx-server`

When `apx-node` is executed with an APX file:

```bash
apx-node vehicle.apx
```

It automatically scans for a companion signature file named `vehicle.apx.sig` (or `vehicle.sig`) in the same directory. If found, `apx-node` automatically attaches the 64-byte signature and negotiates protocol `RMFP/1.1` with `apx-server`.

If `apx-server` is configured with `require-signed-nodes: true` and the corresponding trusted public key, the server cryptographically authenticates the node before accepting its port connections.
