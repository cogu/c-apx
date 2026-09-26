#!/usr/bin/env bash
# ==============================================================================
# Script to generate self-signed ECDSA certificates for APX TLS and mTLS testing.
# Conforms to Item 6 in TODO.md (NIST P-256 / prime256v1 curve).
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${1:-${SCRIPT_DIR}}"
DAYS_VALID=3650 # 10 years
CURVE="prime256v1" # NIST P-256

if ! command -v openssl >/dev/null 2>&1; then
    echo "Error: openssl CLI is required but not found in PATH." >&2
    exit 1
fi

mkdir -p "${OUTPUT_DIR}"
cd "${OUTPUT_DIR}"

echo "=== Generating APX Test TLS Certificates in ${OUTPUT_DIR} ==="

# ------------------------------------------------------------------------------
# STEP 1: Generate Test Root Certificate Authority (CA)
#   - Private key: ca_key.pem
#   - Certificate: ca_cert.pem
# ------------------------------------------------------------------------------
echo "--> [1/3] Generating Root CA (ca_key.pem, ca_cert.pem)..."
openssl ecparam -name "${CURVE}" -genkey -noout -out ca_key.pem

openssl req -new -x509 -key ca_key.pem -sha256 -days "${DAYS_VALID}" \
    -subj "/CN=APX Test Root CA" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" \
    -addext "subjectKeyIdentifier=hash" \
    -out ca_cert.pem

# ------------------------------------------------------------------------------
# STEP 2: Generate Server Certificate (depends on Step 1 CA)
#   - Private key: server_key.pem
#   - Certificate: server_cert.pem (signed by ca_cert.pem with SAN)
# ------------------------------------------------------------------------------
echo "--> [2/3] Generating Server Certificate (server_key.pem, server_cert.pem)..."
openssl ecparam -name "${CURVE}" -genkey -noout -out server_key.pem
openssl req -new -key server_key.pem -subj "/CN=localhost" -out server.csr

cat > server_ext.cnf << 'EOF'
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
IP.1 = 127.0.0.1
IP.2 = ::1
EOF

openssl x509 -req -in server.csr -CA ca_cert.pem -CAkey ca_key.pem -CAcreateserial \
    -out server_cert.pem -days "${DAYS_VALID}" -sha256 \
    -extfile server_ext.cnf

rm -f server.csr server_ext.cnf ca_cert.srl

# ------------------------------------------------------------------------------
# STEP 3: Generate Client Certificate for Mutual TLS (mTLS) (depends on Step 1 CA)
#   - Private key: client_key.pem
#   - Certificate: client_cert.pem (signed by ca_cert.pem)
# ------------------------------------------------------------------------------
echo "--> [3/3] Generating Client Certificate for mTLS (client_key.pem, client_cert.pem)..."
openssl ecparam -name "${CURVE}" -genkey -noout -out client_key.pem
openssl req -new -key client_key.pem -subj "/CN=apx-node-client" -out client.csr

cat > client_ext.cnf << 'EOF'
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature
extendedKeyUsage = clientAuth
EOF

openssl x509 -req -in client.csr -CA ca_cert.pem -CAkey ca_key.pem -CAcreateserial \
    -out client_cert.pem -days "${DAYS_VALID}" -sha256 \
    -extfile client_ext.cnf

rm -f client.csr client_ext.cnf ca_cert.srl

# Export public key for APX file signing verification
openssl ec -in client_key.pem -pubout -out client_pubkey.pem

# Set permissions: private keys read-only by user, public certs world-readable
chmod 600 ca_key.pem server_key.pem client_key.pem
chmod 644 ca_cert.pem server_cert.pem client_cert.pem client_pubkey.pem

# ------------------------------------------------------------------------------
# STEP 4: Verification Check
# ------------------------------------------------------------------------------
echo "--> Verifying generated certificates against Test Root CA..."
openssl verify -CAfile ca_cert.pem server_cert.pem
openssl verify -CAfile ca_cert.pem client_cert.pem

echo "=== Successfully generated and verified test certificates! ==="
ls -lh ca_cert.pem ca_key.pem server_cert.pem server_key.pem client_cert.pem client_key.pem
