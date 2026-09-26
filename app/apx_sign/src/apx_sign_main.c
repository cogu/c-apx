/*****************************************************************************
* \file      apx_sign_main.c
* \author    Conny Gustafsson
* \date      2026-09-27
* \brief     APX cryptographic signing and verification utility
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "adt_str.h"
#include "adt_bytearray.h"
#include "argparse.h"
#include "filestream.h"
#include "fileutil.h"
#include "apx/crypto.h"
#include "apx/remotefile.h"
#include "apx/error.h"
#include "apx_version.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx-sign"

typedef enum apx_sign_mode_tag
{
   APX_SIGN_MODE_SIGN,
   APX_SIGN_MODE_VERIFY,
   APX_SIGN_MODE_KEYGEN
} apx_sign_mode_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_sign_mode_t m_mode = APX_SIGN_MODE_SIGN;
static bool m_display_help = false;
static bool m_display_version = false;
static adt_str_t m_input_file;
static adt_str_t m_key_file;
static adt_str_t m_output_file;
static adt_str_t m_signature_file;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void print_usage(const char *arg0);
static void print_version(void);
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value);
static int do_sign(void);
static int do_verify(void);
static int do_keygen(void);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   adt_str_create(&m_input_file);
   adt_str_create(&m_key_file);
   adt_str_create(&m_output_file);
   adt_str_create(&m_signature_file);

   int arg_start = 1;
   if (argc >= 2)
   {
      if (strcmp(argv[1], "sign") == 0)
      {
         m_mode = APX_SIGN_MODE_SIGN;
         arg_start = 2;
      }
      else if (strcmp(argv[1], "verify") == 0)
      {
         m_mode = APX_SIGN_MODE_VERIFY;
         arg_start = 2;
      }
      else if (strcmp(argv[1], "keygen") == 0 || strcmp(argv[1], "generate-keys") == 0)
      {
         m_mode = APX_SIGN_MODE_KEYGEN;
         arg_start = 2;
      }
   }

   int retval = 0;
   argparse_result_t parse_result = argparse_exec(argc - arg_start + 1, (const char**)&argv[arg_start - 1], argparse_cbk);
   if (parse_result != ARGPARSE_SUCCESS)
   {
      fprintf(stderr, "Error parsing arguments\n");
      print_usage(argv[0]);
      retval = 1;
      goto CLEANUP;
   }

   if (m_display_help)
   {
      print_usage(argv[0]);
      goto CLEANUP;
   }
   if (m_display_version)
   {
      print_version();
      goto CLEANUP;
   }

   switch (m_mode)
   {
   case APX_SIGN_MODE_KEYGEN:
      retval = do_keygen();
      break;
   case APX_SIGN_MODE_VERIFY:
      retval = do_verify();
      break;
   case APX_SIGN_MODE_SIGN:
   default:
      retval = do_sign();
      break;
   }

CLEANUP:
   adt_str_destroy(&m_input_file);
   adt_str_destroy(&m_key_file);
   adt_str_destroy(&m_output_file);
   adt_str_destroy(&m_signature_file);
   return retval;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void print_usage(const char *arg0)
{
   printf("Usage: %s [command] [options] <file.apx>\n", arg0);
   printf("\nCommands:\n");
   printf("  sign               Sign an APX file using an ECDSA P-256 private key (default)\n");
   printf("  verify             Verify an APX file using an ECDSA P-256 public key\n");
   printf("  keygen [prefix]    Generate a new ECDSA P-256 keypair PEM (default prefix: node)\n");
   printf("\nOptions:\n");
   printf("  -k, --key <file>       Private key (sign) or public key (verify) PEM file\n");
   printf("  -o, --output <file>    Output signature file (sign) or key prefix (keygen)\n");
   printf("  -s, --sig <file>       Signature file to verify (defaults to <file.apx>.sig)\n");
   printf("  -v, --verify           Switch to verify mode\n");
   printf("  -g, --keygen           Switch to keygen mode\n");
   printf("  -h, --help             Show this help message\n");
   printf("  -V, --version          Show version information\n");
   printf("\nExamples:\n");
   printf("  %s keygen certs/node\n", arg0);
   printf("  %s -k certs/node_key.pem node.apx\n", arg0);
   printf("  %s verify -k certs/node_pubkey.pem node.apx\n", arg0);
}

static void print_version(void)
{
   printf("%s %s\n", APP_NAME, SW_VERSION_LITERAL);
}

static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value)
{
   if (value == NULL)
   {
      if (short_name != NULL)
      {
         if (strcmp(short_name, "k") == 0 || strcmp(short_name, "o") == 0 || strcmp(short_name, "s") == 0)
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if (strcmp(short_name, "v") == 0)
         {
            m_mode = APX_SIGN_MODE_VERIFY;
         }
         else if (strcmp(short_name, "g") == 0)
         {
            m_mode = APX_SIGN_MODE_KEYGEN;
         }
         else if (strcmp(short_name, "h") == 0)
         {
            m_display_help = true;
         }
         else if (strcmp(short_name, "V") == 0)
         {
            m_display_version = true;
         }
         else
         {
            return ARGPARSE_INVALID_ARGUMENT_ERROR;
         }
      }
      else if (long_name != NULL)
      {
         if (strcmp(long_name, "key") == 0 || strcmp(long_name, "private-key") == 0 ||
             strcmp(long_name, "public-key") == 0 || strcmp(long_name, "output") == 0 ||
             strcmp(long_name, "sig") == 0 || strcmp(long_name, "signature") == 0)
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if (strcmp(long_name, "verify") == 0)
         {
            m_mode = APX_SIGN_MODE_VERIFY;
         }
         else if (strcmp(long_name, "keygen") == 0 || strcmp(long_name, "generate") == 0)
         {
            m_mode = APX_SIGN_MODE_KEYGEN;
         }
         else if (strcmp(long_name, "help") == 0)
         {
            m_display_help = true;
         }
         else if (strcmp(long_name, "version") == 0)
         {
            m_display_version = true;
         }
         else
         {
            return ARGPARSE_INVALID_ARGUMENT_ERROR;
         }
      }
      return ARGPARSE_SUCCESS;
   }
   else
   {
      if (short_name != NULL)
      {
         if (strcmp(short_name, "k") == 0)
         {
            adt_str_set_cstr(&m_key_file, value);
         }
         else if (strcmp(short_name, "o") == 0)
         {
            adt_str_set_cstr(&m_output_file, value);
         }
         else if (strcmp(short_name, "s") == 0)
         {
            adt_str_set_cstr(&m_signature_file, value);
         }
         else
         {
            return ARGPARSE_INVALID_ARGUMENT_ERROR;
         }
      }
      else if (long_name != NULL)
      {
         if (strcmp(long_name, "key") == 0 || strcmp(long_name, "private-key") == 0 ||
             strcmp(long_name, "public-key") == 0)
         {
            adt_str_set_cstr(&m_key_file, value);
         }
         else if (strcmp(long_name, "output") == 0)
         {
            adt_str_set_cstr(&m_output_file, value);
         }
         else if (strcmp(long_name, "sig") == 0 || strcmp(long_name, "signature") == 0)
         {
            adt_str_set_cstr(&m_signature_file, value);
         }
         else
         {
            return ARGPARSE_INVALID_ARGUMENT_ERROR;
         }
      }
      else
      {
         // Positional argument
         if (adt_str_length(&m_input_file) == 0)
         {
            adt_str_set_cstr(&m_input_file, value);
         }
      }
      return ARGPARSE_SUCCESS;
   }
}

static int do_sign(void)
{
   if (adt_str_length(&m_key_file) == 0)
   {
      fprintf(stderr, "Error: Private key file (-k/--key) is required for signing\n");
      return 1;
   }
   if (adt_str_length(&m_input_file) == 0)
   {
      fprintf(stderr, "Error: No input .apx file specified\n");
      return 1;
   }

   const char *apx_path = adt_str_cstr(&m_input_file);
   adt_bytearray_t *apx_bytes = cutil_ifstream_util_read_text_file(apx_path);
   if (apx_bytes == NULL)
   {
      fprintf(stderr, "Error: Failed to read APX file: %s\n", apx_path);
      return 1;
   }

   const char *key_path = adt_str_cstr(&m_key_file);
   adt_bytearray_t *key_bytes = cutil_ifstream_util_read_text_file(key_path);
   if (key_bytes == NULL)
   {
      fprintf(stderr, "Error: Failed to read private key file: %s\n", key_path);
      adt_bytearray_delete(apx_bytes);
      return 1;
   }

   uint8_t sig_buf[RMF_SIGNATURE_SIZE_ECDSA_P256];
   size_t sig_len = 0;
   apx_error_t rc = apx_crypto_sign_data(RMF_SIGNATURE_TYPE_ECDSA_P256,
                                         (const uint8_t*)adt_bytearray_data(key_bytes),
                                         (size_t)adt_bytearray_length(key_bytes),
                                         (const uint8_t*)adt_bytearray_data(apx_bytes),
                                         (size_t)adt_bytearray_length(apx_bytes),
                                         sig_buf, sizeof(sig_buf), &sig_len);
   adt_bytearray_delete(apx_bytes);
   adt_bytearray_delete(key_bytes);

   if (rc != APX_NO_ERROR)
   {
      fprintf(stderr, "Error: Cryptographic signing failed (error code %d)\n", (int)rc);
      return 1;
   }

   adt_str_t *out_path = NULL;
   if (adt_str_length(&m_output_file) > 0)
   {
      out_path = adt_str_clone(&m_output_file);
   }
   else
   {
      out_path = cutil_path_append_extension(apx_path, ".sig");
   }

   if (out_path == NULL)
   {
      fprintf(stderr, "Error: Memory allocation failure\n");
      return 1;
   }

   if (cutil_write_binary_file(adt_str_cstr(out_path), sig_buf, sig_len) != 0)
   {
      fprintf(stderr, "Error: Failed to write signature file: %s\n", adt_str_cstr(out_path));
      adt_str_delete(out_path);
      return 1;
   }

   printf("Signed '%s' -> '%s' (%u bytes)\n", apx_path, adt_str_cstr(out_path), (unsigned int)sig_len);
   adt_str_delete(out_path);
   return 0;
}

static int do_verify(void)
{
   if (adt_str_length(&m_key_file) == 0)
   {
      fprintf(stderr, "Error: Public key file (-k/--key) is required for verification\n");
      return 1;
   }
   if (adt_str_length(&m_input_file) == 0)
   {
      fprintf(stderr, "Error: No input .apx file specified\n");
      return 1;
   }

   const char *apx_path = adt_str_cstr(&m_input_file);
   adt_bytearray_t *apx_bytes = cutil_ifstream_util_read_text_file(apx_path);
   if (apx_bytes == NULL)
   {
      fprintf(stderr, "Error: Failed to read APX file: %s\n", apx_path);
      return 1;
   }

   const char *key_path = adt_str_cstr(&m_key_file);
   adt_bytearray_t *key_bytes = cutil_ifstream_util_read_text_file(key_path);
   if (key_bytes == NULL)
   {
      fprintf(stderr, "Error: Failed to read public key file: %s\n", key_path);
      adt_bytearray_delete(apx_bytes);
      return 1;
   }

   adt_str_t *sig_path = NULL;
   if (adt_str_length(&m_signature_file) > 0)
   {
      sig_path = adt_str_clone(&m_signature_file);
   }
   else
   {
      sig_path = cutil_path_append_extension(apx_path, ".sig");
      if (sig_path != NULL && !cutil_file_exists(adt_str_cstr(sig_path)))
      {
         adt_str_delete(sig_path);
         sig_path = cutil_path_replace_extension(apx_path, ".sig");
      }
   }

   if (sig_path == NULL || !cutil_file_exists(adt_str_cstr(sig_path)))
   {
      fprintf(stderr, "Error: Signature file not found for '%s'\n", apx_path);
      if (sig_path != NULL) adt_str_delete(sig_path);
      adt_bytearray_delete(apx_bytes);
      adt_bytearray_delete(key_bytes);
      return 1;
   }

   uint8_t sig_buf[RMF_SIGNATURE_SIZE_ECDSA_P256];
   size_t bytes_read = 0;
   if (cutil_read_binary_file(adt_str_cstr(sig_path), sig_buf, sizeof(sig_buf), &bytes_read) != 0 ||
       bytes_read != sizeof(sig_buf))
   {
      fprintf(stderr, "Error: Failed to read valid 64-byte signature from: %s\n", adt_str_cstr(sig_path));
      adt_str_delete(sig_path);
      adt_bytearray_delete(apx_bytes);
      adt_bytearray_delete(key_bytes);
      return 1;
   }

   apx_error_t rc = apx_crypto_verify_signature(RMF_SIGNATURE_TYPE_ECDSA_P256,
                                                (const uint8_t*)adt_bytearray_data(key_bytes),
                                                (size_t)adt_bytearray_length(key_bytes),
                                                (const uint8_t*)adt_bytearray_data(apx_bytes),
                                                (size_t)adt_bytearray_length(apx_bytes),
                                                sig_buf, bytes_read);

   if (rc == APX_NO_ERROR)
   {
      printf("OK: Signature in '%s' is valid for '%s'\n", adt_str_cstr(sig_path), apx_path);
   }
   else
   {
      fprintf(stderr, "FAILED: Signature verification failed for '%s' (error code %d)\n", apx_path, (int)rc);
   }

   adt_str_delete(sig_path);
   adt_bytearray_delete(apx_bytes);
   adt_bytearray_delete(key_bytes);
   return (rc == APX_NO_ERROR) ? 0 : 1;
}

static int do_keygen(void)
{
   const char *prefix = "node";
   if (adt_str_length(&m_output_file) > 0)
   {
      prefix = adt_str_cstr(&m_output_file);
   }
   else if (adt_str_length(&m_input_file) > 0)
   {
      prefix = adt_str_cstr(&m_input_file);
   }

   char *priv_pem = NULL;
   char *pub_pem = NULL;
   apx_error_t rc = apx_crypto_generate_keypair_pem(&priv_pem, &pub_pem);
   if (rc != APX_NO_ERROR || priv_pem == NULL || pub_pem == NULL)
   {
      fprintf(stderr, "Error: Keypair generation failed (error code %d)\n", (int)rc);
      return 1;
   }

   adt_str_t *priv_path = adt_str_new_cstr(prefix);
   adt_str_append_cstr(priv_path, "_key.pem");

   adt_str_t *pub_path = adt_str_new_cstr(prefix);
   adt_str_append_cstr(pub_path, "_pubkey.pem");

   int result = 0;
   if (cutil_write_binary_file(adt_str_cstr(priv_path), (const uint8_t*)priv_pem, strlen(priv_pem)) != 0)
   {
      fprintf(stderr, "Error: Failed to write private key to '%s'\n", adt_str_cstr(priv_path));
      result = 1;
   }
   else if (cutil_write_binary_file(adt_str_cstr(pub_path), (const uint8_t*)pub_pem, strlen(pub_pem)) != 0)
   {
      fprintf(stderr, "Error: Failed to write public key to '%s'\n", adt_str_cstr(pub_path));
      result = 1;
   }
   else
   {
      printf("Generated ECDSA NIST P-256 keypair:\n");
      printf("  Private key: %s\n", adt_str_cstr(priv_path));
      printf("  Public key:  %s\n", adt_str_cstr(pub_path));
   }

   adt_str_delete(priv_path);
   adt_str_delete(pub_path);
   free(priv_pem);
   free(pub_pem);
   return result;
}
