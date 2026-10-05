/*****************************************************************************
 * \file      crypto.h
 * \author    Conny Gustafsson
 * \date      2026-09-26
 * \brief     Cryptographic signing and verification utilities (Mbed TLS)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_CRYPTO_H
#define APX_CRYPTO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/remotefile.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * Computes SHA-256 hash of data buffer.
 * \param data Pointer to input data
 * \param data_len Length of input data in bytes
 * \param hash Output buffer (must be at least 32 bytes)
 */
apx_error_t apx_crypto_sha256(const uint8_t *data, size_t data_len, uint8_t hash[32]);

/**
 * Verifies an APX signature over data using a public key.
 *
 * Supported signature types:
 * - RMF_SIGNATURE_TYPE_ECDSA_P256: 64-byte raw (r, s) signature.
 *
 * Supported public key formats:
 * - PEM string (e.g. "-----BEGIN PUBLIC KEY-----...")
 * - DER encoded SubjectPublicKeyInfo
 * - Raw uncompressed EC point (65 bytes: 0x04 || X || Y)
 *
 * \param sig_type Signature type (e.g. RMF_SIGNATURE_TYPE_ECDSA_P256)
 * \param pub_key Public key bytes (PEM or binary)
 * \param pub_key_len Public key length
 * \param data Payload data that was signed
 * \param data_len Length of payload data
 * \param sig_data Raw signature bytes (64 bytes for ECDSA P-256)
 * \param sig_len Length of signature bytes (must be 64 for ECDSA P-256)
 *
 * \return APX_NO_ERROR if signature is valid.
 * \return APX_SIGNATURE_VERIFICATION_ERROR if signature does not match.
 * \return APX_INVALID_ARGUMENT_ERROR if arguments or key are invalid.
 */
apx_error_t apx_crypto_verify_signature(rmf_signature_type_t sig_type, const uint8_t *pub_key, size_t pub_key_len,
  const uint8_t *data, size_t data_len, const uint8_t *sig_data, size_t sig_len);

/**
 * Signs data using a private key and outputs a raw APX signature.
 *
 * Supported signature types:
 * - RMF_SIGNATURE_TYPE_ECDSA_P256: produces 64-byte raw (r, s) signature using deterministic ECDSA (RFC 6979).
 *
 * Supported private key formats:
 * - PEM string (e.g. "-----BEGIN EC PRIVATE KEY-----..." or "-----BEGIN PRIVATE KEY-----...")
 * - DER encoded private key
 *
 * \param sig_type Signature type
 * \param priv_key Private key bytes (PEM or binary)
 * \param priv_key_len Private key length
 * \param data Payload data to sign
 * \param data_len Length of payload data
 * \param sig_data Output buffer for signature (must be at least 64 bytes for ECDSA P-256)
 * \param sig_buf_size Size of sig_data buffer
 * \param sig_len Output pointer for actual signature length
 *
 * \return APX_NO_ERROR on success.
 */
apx_error_t apx_crypto_sign_data(rmf_signature_type_t sig_type, const uint8_t *priv_key, size_t priv_key_len,
  const uint8_t *data, size_t data_len, uint8_t *sig_data, size_t sig_buf_size, size_t *sig_len);

/**
 * Helper to generate a new ECDSA P-256 keypair in PEM format.
 * Dynamically allocates strings; caller must free() *priv_key_pem and *pub_key_pem.
 */
apx_error_t apx_crypto_generate_keypair_pem(char **priv_key_pem, char **pub_key_pem);

#endif // APX_CRYPTO_H
