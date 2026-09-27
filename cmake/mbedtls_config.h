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
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_SHA256_C

/* RAM / ROM optimization */
#define MBEDTLS_MPI_MAX_SIZE          32  /* 256-bit EC = 32 bytes */
#define MBEDTLS_ECP_WINDOW_SIZE       2
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 0
#define MBEDTLS_ENTROPY_MAX_SOURCES   2

#endif /* APX_MBEDTLS_CONFIG_H */
