/*****************************************************************************
* \file      mbedtls_config.h
* \author    Conny Gustafsson
* \date      2026-09-27
* \brief     Tailored minimal Mbed TLS configuration for c-apx
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_MBEDTLS_CONFIG_H
#define APX_MBEDTLS_CONFIG_H

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_FS_IO

/* Curve & Algorithm */
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ECDSA_DETERMINISTIC
#define MBEDTLS_HMAC_DRBG_C

/* Modules */
#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CCM_C
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C

/* TLS & X.509 */
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_ALPN
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

/* RAM / ROM optimization */
#define MBEDTLS_MPI_MAX_SIZE          1024
#define MBEDTLS_ECP_WINDOW_SIZE       2
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 0
#define MBEDTLS_ENTROPY_MAX_SOURCES   2

#endif /* APX_MBEDTLS_CONFIG_H */
