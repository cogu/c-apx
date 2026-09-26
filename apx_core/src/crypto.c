/*****************************************************************************
* \file      crypto.c
* \author    Conny Gustafsson
* \date      2026-09-26
* \brief     Cryptographic signing and verification utilities (Mbed TLS)
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/crypto.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char* m_drbg_pers = "apx_crypto_drbg";

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t verify_ecdsa_p256(const uint8_t* pub_key, size_t pub_key_len,
                                     const uint8_t hash[32],
                                     const uint8_t* sig_data, size_t sig_len);

static apx_error_t sign_ecdsa_p256(const uint8_t* priv_key, size_t priv_key_len,
                                   const uint8_t hash[32],
                                   uint8_t* sig_data, size_t sig_buf_size,
                                   size_t* sig_len);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_crypto_sha256(const uint8_t* data, size_t data_len, uint8_t hash[32])
{
   if ((data == NULL && data_len > 0) || hash == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   int ret = mbedtls_sha256(data, data_len, hash, 0);
   if (ret != 0)
   {
      return APX_INTERNAL_ERROR;
   }
   return APX_NO_ERROR;
}

apx_error_t apx_crypto_verify_signature(rmf_signature_type_t sig_type,
                                        const uint8_t* pub_key, size_t pub_key_len,
                                        const uint8_t* data, size_t data_len,
                                        const uint8_t* sig_data, size_t sig_len)
{
   if (pub_key == NULL || pub_key_len == 0 || (data == NULL && data_len > 0) ||
       sig_data == NULL || sig_len == 0)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }

   uint8_t hash[32];
   apx_error_t err = apx_crypto_sha256(data, data_len, hash);
   if (err != APX_NO_ERROR)
   {
      return err;
   }

   switch (sig_type)
   {
   case RMF_SIGNATURE_TYPE_ECDSA_P256:
      return verify_ecdsa_p256(pub_key, pub_key_len, hash, sig_data, sig_len);
   default:
      return APX_UNSUPPORTED_ERROR;
   }
}

apx_error_t apx_crypto_sign_data(rmf_signature_type_t sig_type,
                                 const uint8_t* priv_key, size_t priv_key_len,
                                 const uint8_t* data, size_t data_len,
                                 uint8_t* sig_data, size_t sig_buf_size,
                                 size_t* sig_len)
{
   if (priv_key == NULL || priv_key_len == 0 || (data == NULL && data_len > 0) ||
       sig_data == NULL || sig_buf_size == 0 || sig_len == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }

   uint8_t hash[32];
   apx_error_t err = apx_crypto_sha256(data, data_len, hash);
   if (err != APX_NO_ERROR)
   {
      return err;
   }

   switch (sig_type)
   {
   case RMF_SIGNATURE_TYPE_ECDSA_P256:
      return sign_ecdsa_p256(priv_key, priv_key_len, hash, sig_data, sig_buf_size, sig_len);
   default:
      return APX_UNSUPPORTED_ERROR;
   }
}

apx_error_t apx_crypto_generate_keypair_pem(char** priv_key_pem, char** pub_key_pem)
{
   if (priv_key_pem == NULL || pub_key_pem == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   *priv_key_pem = NULL;
   *pub_key_pem = NULL;

   mbedtls_pk_context pk;
   mbedtls_entropy_context entropy;
   mbedtls_ctr_drbg_context ctr_drbg;
   mbedtls_pk_init(&pk);
   mbedtls_entropy_init(&entropy);
   mbedtls_ctr_drbg_init(&ctr_drbg);

   int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char*)m_drbg_pers, strlen(m_drbg_pers));
   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INTERNAL_ERROR;
   }

   ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INTERNAL_ERROR;
   }

   ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(pk),
                             mbedtls_ctr_drbg_random, &ctr_drbg);
   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INTERNAL_ERROR;
   }

   char priv_buf[1600];
   char pub_buf[1600];
   memset(priv_buf, 0, sizeof(priv_buf));
   memset(pub_buf, 0, sizeof(pub_buf));

   ret = mbedtls_pk_write_key_pem(&pk, (unsigned char*)priv_buf, sizeof(priv_buf));
   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INTERNAL_ERROR;
   }

   ret = mbedtls_pk_write_pubkey_pem(&pk, (unsigned char*)pub_buf, sizeof(pub_buf));
   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INTERNAL_ERROR;
   }

   size_t priv_len = strlen(priv_buf);
   size_t pub_len = strlen(pub_buf);
   char* priv_copy = (char*)malloc(priv_len + 1);
   char* pub_copy = (char*)malloc(pub_len + 1);
   if (priv_copy == NULL || pub_copy == NULL)
   {
      free(priv_copy);
      free(pub_copy);
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_MEM_ERROR;
   }
   memcpy(priv_copy, priv_buf, priv_len + 1);
   memcpy(pub_copy, pub_buf, pub_len + 1);

   *priv_key_pem = priv_copy;
   *pub_key_pem = pub_copy;

   mbedtls_ctr_drbg_free(&ctr_drbg);
   mbedtls_entropy_free(&entropy);
   mbedtls_pk_free(&pk);
   return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t verify_ecdsa_p256(const uint8_t* pub_key, size_t pub_key_len,
                                     const uint8_t hash[32],
                                     const uint8_t* sig_data, size_t sig_len)
{
   if (sig_len != RMF_SIGNATURE_SIZE_ECDSA_P256)
   {
      return APX_LENGTH_ERROR;
   }

   mbedtls_mpi r;
   mbedtls_mpi s;
   mbedtls_mpi_init(&r);
   mbedtls_mpi_init(&s);

   int ret = mbedtls_mpi_read_binary(&r, sig_data, 32);
   if (ret == 0)
   {
      ret = mbedtls_mpi_read_binary(&s, sig_data + 32, 32);
   }
   if (ret != 0)
   {
      mbedtls_mpi_free(&r);
      mbedtls_mpi_free(&s);
      return APX_INVALID_ARGUMENT_ERROR;
   }

   // 1. Try parsing using PK context (PEM or DER)
   mbedtls_pk_context pk;
   mbedtls_pk_init(&pk);

   // If the key is in PEM format (starts with "---"), Mbed TLS expects a null-terminated string
   char* pem_copy = NULL;
   const unsigned char* key_to_parse = pub_key;
   size_t key_len_to_parse = pub_key_len;

   if (pub_key_len > 5 && strstr((const char*)pub_key, "-----BEGIN") != NULL)
   {
      if (pub_key[pub_key_len - 1] != '\0')
      {
         pem_copy = (char*)malloc(pub_key_len + 1);
         if (pem_copy == NULL)
         {
            mbedtls_mpi_free(&r);
            mbedtls_mpi_free(&s);
            mbedtls_pk_free(&pk);
            return APX_MEM_ERROR;
         }
         memcpy(pem_copy, pub_key, pub_key_len);
         pem_copy[pub_key_len] = '\0';
         key_to_parse = (const unsigned char*)pem_copy;
         key_len_to_parse = pub_key_len + 1;
      }
   }

   ret = mbedtls_pk_parse_public_key(&pk, key_to_parse, key_len_to_parse);
   if (pem_copy != NULL)
   {
      free(pem_copy);
      pem_copy = NULL;
   }

   if (ret == 0)
   {
      if (mbedtls_pk_can_do(&pk, MBEDTLS_PK_ECDSA))
      {
         mbedtls_ecp_keypair* eckey = mbedtls_pk_ec(pk);
         if (eckey->grp.id == MBEDTLS_ECP_DP_SECP256R1)
         {
            int verify_ret = mbedtls_ecdsa_verify(&eckey->grp, hash, 32, &eckey->Q, &r, &s);
            mbedtls_mpi_free(&r);
            mbedtls_mpi_free(&s);
            mbedtls_pk_free(&pk);
            return (verify_ret == 0) ? APX_NO_ERROR : APX_SIGNATURE_VERIFICATION_ERROR;
         }
      }
   }
   mbedtls_pk_free(&pk);

   // 2. Fallback: Raw uncompressed EC point (65 bytes: 0x04 || X || Y)
   if (pub_key_len == 65 && pub_key[0] == 0x04)
   {
      mbedtls_ecp_group grp;
      mbedtls_ecp_point Q;
      mbedtls_ecp_group_init(&grp);
      mbedtls_ecp_point_init(&Q);

      ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
      if (ret == 0)
      {
         ret = mbedtls_ecp_point_read_binary(&grp, &Q, pub_key, pub_key_len);
         if (ret == 0)
         {
            int verify_ret = mbedtls_ecdsa_verify(&grp, hash, 32, &Q, &r, &s);
            mbedtls_ecp_point_free(&Q);
            mbedtls_ecp_group_free(&grp);
            mbedtls_mpi_free(&r);
            mbedtls_mpi_free(&s);
            return (verify_ret == 0) ? APX_NO_ERROR : APX_SIGNATURE_VERIFICATION_ERROR;
         }
      }
      mbedtls_ecp_point_free(&Q);
      mbedtls_ecp_group_free(&grp);
   }

   mbedtls_mpi_free(&r);
   mbedtls_mpi_free(&s);
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t sign_ecdsa_p256(const uint8_t* priv_key, size_t priv_key_len,
                                   const uint8_t hash[32],
                                   uint8_t* sig_data, size_t sig_buf_size,
                                   size_t* sig_len)
{
   if (sig_buf_size < RMF_SIGNATURE_SIZE_ECDSA_P256)
   {
      return APX_LENGTH_ERROR;
   }

   mbedtls_entropy_context entropy;
   mbedtls_ctr_drbg_context ctr_drbg;
   mbedtls_entropy_init(&entropy);
   mbedtls_ctr_drbg_init(&ctr_drbg);

   int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char*)m_drbg_pers, strlen(m_drbg_pers));
   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      return APX_INTERNAL_ERROR;
   }

   mbedtls_pk_context pk;
   mbedtls_pk_init(&pk);

   char* pem_copy = NULL;
   const unsigned char* key_to_parse = priv_key;
   size_t key_len_to_parse = priv_key_len;

   if (priv_key_len > 5 && strstr((const char*)priv_key, "-----BEGIN") != NULL)
   {
      if (priv_key[priv_key_len - 1] != '\0')
      {
         pem_copy = (char*)malloc(priv_key_len + 1);
         if (pem_copy == NULL)
         {
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);
            mbedtls_pk_free(&pk);
            return APX_MEM_ERROR;
         }
         memcpy(pem_copy, priv_key, priv_key_len);
         pem_copy[priv_key_len] = '\0';
         key_to_parse = (const unsigned char*)pem_copy;
         key_len_to_parse = priv_key_len + 1;
      }
   }

   ret = mbedtls_pk_parse_key(&pk, key_to_parse, key_len_to_parse, NULL, 0,
                              mbedtls_ctr_drbg_random, &ctr_drbg);
   if (pem_copy != NULL)
   {
      free(pem_copy);
      pem_copy = NULL;
   }

   if (ret != 0)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INVALID_ARGUMENT_ERROR;
   }

   if (!mbedtls_pk_can_do(&pk, MBEDTLS_PK_ECDSA))
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_UNSUPPORTED_ERROR;
   }

   mbedtls_ecp_keypair* eckey = mbedtls_pk_ec(pk);
   if (eckey->grp.id != MBEDTLS_ECP_DP_SECP256R1)
   {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_UNSUPPORTED_ERROR;
   }

   mbedtls_mpi r;
   mbedtls_mpi s;
   mbedtls_mpi_init(&r);
   mbedtls_mpi_init(&s);

   ret = mbedtls_ecdsa_sign_det_ext(&eckey->grp, &r, &s, &eckey->d, hash, 32,
                                    MBEDTLS_MD_SHA256,
                                    mbedtls_ctr_drbg_random, &ctr_drbg);
   if (ret != 0)
   {
      mbedtls_mpi_free(&r);
      mbedtls_mpi_free(&s);
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedtls_pk_free(&pk);
      return APX_INTERNAL_ERROR;
   }

   ret = mbedtls_mpi_write_binary(&r, sig_data, 32);
   if (ret == 0)
   {
      ret = mbedtls_mpi_write_binary(&s, sig_data + 32, 32);
   }

   mbedtls_mpi_free(&r);
   mbedtls_mpi_free(&s);
   mbedtls_ctr_drbg_free(&ctr_drbg);
   mbedtls_entropy_free(&entropy);
   mbedtls_pk_free(&pk);

   if (ret != 0)
   {
      return APX_INTERNAL_ERROR;
   }

   *sig_len = RMF_SIGNATURE_SIZE_ECDSA_P256;
   return APX_NO_ERROR;
}
