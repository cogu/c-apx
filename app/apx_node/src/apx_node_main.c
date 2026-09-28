/*****************************************************************************
* \file      apx_node_main.c
* \author    Conny Gustafsson
* \date      2020-04-12
* \brief     apx_node console application
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#endif
#include <assert.h>
#include "adt_str.h"
#include "apx_connection.h"
#include "argparse.h"
#include "msocket.h"
#if defined(MSOCKET_ENABLE_TLS)
#include "msocket_tls.h"
#endif
#include "json_server.h"
#include "filestream.h"
#include "fileutil.h"
#include "apx_version.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx-node"
#define EXAMPLE_CA_CERT_PATH     "example/secure/config/certs/ca_cert.pem"
#define EXAMPLE_CLIENT_CERT_PATH "example/secure/config/certs/client_cert.pem"
#define EXAMPLE_CLIENT_KEY_PATH  "example/secure/config/certs/client_key.pem"
#define PYTEST_CA_CERT_PATH      "tests/certs/ca_cert.pem"
#define PYTEST_CLIENT_CERT_PATH  "tests/certs/client_cert.pem"
#define PYTEST_CLIENT_KEY_PATH   "tests/certs/client_key.pem"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value);
static adt_str_t *read_definition_file(adt_str_t *path);
static void print_version(void);
static void print_usage(const char *arg0);
static void application_shutdown(void);
static void application_cleanup(void);
#ifndef _WIN32
static void signal_handler_setup(void);
static void signal_handler(int signum);
#else
static int init_wsa(void);
#endif
static apx_error_t init_json_message_server(void);
static apx_error_t connect_to_apx_server(void);
static apx_error_t start_json_message_server(void);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

/*** Argument variables ***/
static const uint16_t bind_port_default = 5100;
static const uint16_t connect_port_default = 5000;
static const uint16_t connect_port_tls_default = 5020;
#ifdef _WIN32
static const char *m_bind_address_default = "127.0.0.1";
static const char *m_connect_address_default = "127.0.0.1";
#else
static const char *m_bind_address_default = "/tmp/apx_node.socket";
static const char *m_connect_address_default = "/tmp/apx.socket";
#endif
static bool m_no_bind = false;
static bool m_no_signature = false;
static bool m_display_help = false;
static bool m_display_version = false;
static bool m_use_tls = false;
static bool m_use_vsock = false;
static uint32_t m_vsock_cid = MSOCKET_VMADDR_CID_ANY;
static uint32_t m_vsock_port = 0u;
static bool m_connect_port_set = false;
static uint16_t m_bind_port;
static uint16_t m_connect_port;
static adt_str_t *m_bind_address = NULL;
static adt_str_t *m_connect_address = NULL;
static adt_str_t *m_ca_cert = NULL;
static adt_str_t *m_client_cert = NULL;
static adt_str_t *m_client_key = NULL;
static adt_str_t m_definition_file;
static msocket_endpoint_type_t m_bind_resource_type = MSOCKET_ENDPOINT_UNKNOWN;
static msocket_endpoint_type_t m_connect_resource_type = MSOCKET_ENDPOINT_UNKNOWN;

/*** Other local variables***/
static adt_str_t *m_apx_definition_str = NULL;
static apx_connection_t *m_apx_connection = NULL;
static int m_runFlag = 1;
static bool m_messageServerRunning = false;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   m_bind_port = bind_port_default;
   m_connect_port = connect_port_default;
   adt_str_create(&m_definition_file);
   int retval = 0;
   argparse_result_t result = argparse_exec(argc, (const char**) argv, argparse_cbk);
   if (result == ARGPARSE_SUCCESS)
   {
      if (m_bind_resource_type == MSOCKET_ENDPOINT_UNKNOWN)
      {
         uint16_t dummy_port;
         m_bind_resource_type = msocket_parse_endpoint(m_bind_address_default, &m_bind_address, &dummy_port);
         (void) dummy_port;
         assert( (m_bind_resource_type != MSOCKET_ENDPOINT_UNKNOWN) && (m_bind_resource_type != MSOCKET_ENDPOINT_ERROR) );
      }
      if (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN)
      {
         uint16_t dummy_port;
         const char *default_connect_addr = (m_use_tls) ? "127.0.0.1" : m_connect_address_default;
         m_connect_resource_type = msocket_parse_endpoint(default_connect_addr, &m_connect_address, &dummy_port);
         (void) dummy_port;
         assert( (m_connect_resource_type != MSOCKET_ENDPOINT_UNKNOWN) && (m_connect_resource_type != MSOCKET_ENDPOINT_ERROR) );
      }
      if (m_use_tls && !m_connect_port_set)
      {
         m_connect_port = connect_port_tls_default;
      }
      if (m_use_vsock && m_use_tls)
      {
         fprintf(stderr, "Error: TLS over VSOCK is not supported\n");
         retval = 1;
         goto SHUTDOWN;
      }
      if (m_display_version)
      {
         print_version();
      }
      if (m_display_help)
      {
         print_usage(argv[0]);
      }
      if (adt_str_length(&m_definition_file) == 0)
      {
         if (!m_display_version && !m_display_help)
         {
            printf("Error: No definition file given\n");
            print_usage(argv[0]);
            retval = 1;
         }
      }
      else
      {
         printf("Initializing APX connection...");
         m_apx_connection = apx_connection_new();
         if (m_apx_connection != NULL)
         {
            printf("OK\n");
         }
         else
         {
            printf("Failed\n");
            retval = 1;
            goto SHUTDOWN;
         }
         m_apx_definition_str = read_definition_file(&m_definition_file);
         if (m_apx_definition_str != NULL)
         {
            uint8_t signature_data[RMF_SIGNATURE_SIZE_ECDSA_P256];
            bool has_signature = false;
            if (!m_no_signature)
            {
               adt_str_t *sig_path = cutil_path_append_extension(adt_str_cstr(&m_definition_file), ".sig");
               if (sig_path != NULL)
               {
                  if (cutil_file_exists(adt_str_cstr(sig_path)))
                  {
                     size_t bytes_read = 0;
                     if (cutil_read_binary_file(adt_str_cstr(sig_path), signature_data, sizeof(signature_data), &bytes_read) == 0 &&
                         bytes_read == sizeof(signature_data))
                     {
                        has_signature = true;
                        printf("Found signature: %s\n", adt_str_cstr(sig_path));
                     }
                  }
                  adt_str_delete(sig_path);
               }
               if (!has_signature)
               {
                  sig_path = cutil_path_replace_extension(adt_str_cstr(&m_definition_file), ".sig");
                  if (sig_path != NULL)
                  {
                     if (cutil_file_exists(adt_str_cstr(sig_path)))
                     {
                        size_t bytes_read = 0;
                        if (cutil_read_binary_file(adt_str_cstr(sig_path), signature_data, sizeof(signature_data), &bytes_read) == 0 &&
                            bytes_read == sizeof(signature_data))
                        {
                           has_signature = true;
                           printf("Found signature: %s\n", adt_str_cstr(sig_path));
                        }
                     }
                     adt_str_delete(sig_path);
                  }
               }
            }

            printf("Parsing %s (%d bytes)...", adt_str_cstr(&m_definition_file), adt_str_size(m_apx_definition_str));
            apx_error_t rc;
            if (has_signature)
            {
               rc = apx_connection_attach_node_signed(m_apx_connection, m_apx_definition_str, RMF_SIGNATURE_TYPE_ECDSA_P256, signature_data);
            }
            else
            {
               rc = apx_connection_attach_node(m_apx_connection, m_apx_definition_str);
            }
            if (rc != APX_NO_ERROR)
            {
               if (rc == APX_PARSE_ERROR)
               {
                  int32_t errorLine = apx_connection_get_last_error_line(m_apx_connection);
                  printf("Failed\n");
                  fprintf(stderr, "Error: Parse error on line %d\n", (int) errorLine);
               }
               else
               {
                  printf("Failed\n");
                  fprintf(stderr, "Error: attach node failed with error code %d\n", (int) rc);
               }
               return 1;
            }
            else
            {
               apx_node_instance_t *node_instance;
               apx_size_t num_provide_ports;
               apx_size_t num_require_ports;
               printf("OK\n");
               node_instance = apx_connection_get_last_attached_node(m_apx_connection);
               if (node_instance != NULL)
               {
                  num_provide_ports = apx_node_instance_get_num_provide_ports(node_instance);
                  num_require_ports = apx_node_instance_get_num_require_ports(node_instance);
                  printf("\t%s: Provide-Ports: %d, Require-Ports: %d\n",
                        apx_node_instance_get_name(node_instance),
                        (int) num_provide_ports, (int) num_require_ports);
               }
               if (m_use_vsock)
               {
                  printf("Connecting to APX server at vsock://%u:%u...", (unsigned int) m_vsock_cid, (unsigned int) m_vsock_port);
               }
               else
               {
                  printf("Connecting to APX server at %s...", adt_str_cstr(m_connect_address));
               }
               rc = connect_to_apx_server();
               if (rc == APX_NO_ERROR)
               {
#ifndef _WIN32
                  sigset_t mask, oldmask;
#endif
                  printf("OK\n");
                  if (!m_no_bind)
                  {
                     printf("Initializing JSON message server...");
                     rc = init_json_message_server();
                     if (rc == APX_NO_ERROR)
                     {
                        printf("OK\n");
                     }
                     else
                     {
                        printf("Failed (%d)\n", (int) rc);
                        goto SHUTDOWN;
                     }
                     printf("Starting JSON message server at \"%s\"...", adt_str_cstr(m_bind_address));
                     rc = start_json_message_server();
                     if (rc == APX_NO_ERROR)
                     {
                        printf("OK\n");
                        m_messageServerRunning = true;
                     }
                     else
                     {
                        printf("Failed (%d)\n", (int) rc);
                        goto SHUTDOWN;
                     }
                  }
#ifdef _WIN32
                  while(m_runFlag)
                  {
                     SLEEP(1);
                  }
#else
                  signal_handler_setup();
                  sigemptyset(&mask);
                  sigaddset(&mask, SIGINT);
                  sigaddset(&mask, SIGTERM);
                  sigprocmask(SIG_BLOCK, &mask, &oldmask);
                  while(m_runFlag)
                  {
                     sigsuspend(&oldmask);
                  }
                  sigprocmask(SIG_UNBLOCK, &mask, NULL);
#endif
               }
               else
               {
                  printf("Failed (%d)\n", (int) rc);
                  retval = 1;
               }
            }
         }
         else
         {
            fprintf(stderr, "Error: Could not read file '%s'\n", adt_str_cstr(&m_definition_file));
            retval = 1;
            goto SHUTDOWN;
         }
      }
   }
   else
   {
      printf("Error parsing argument (%d)\n", (int) result);
      print_usage(argv[0]);
      retval = 1;
   }
SHUTDOWN:
   application_shutdown();
   application_cleanup();
   return retval;
}

#ifdef MEM_LEAK_CHECK
void vfree(void *arg)
{
   free(arg);
}
#endif
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value)
{
   if (value == NULL)
   {
      if ( short_name != NULL )
      {
         if ( (strcmp(short_name,"b")==0) || (strcmp(short_name,"p")==0) ||
              (strcmp(short_name,"c")==0) || (strcmp(short_name,"r")==0) )
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if( (strcmp(short_name,"h")==0) )
         {
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else
         {
            return ARGPARSE_NAME_ERROR;
         }
      }
      else if ( (long_name != NULL) )
      {
         if ( (strcmp(long_name,"bind")==0) || (strcmp(long_name,"bind-port")==0) ||
              (strcmp(long_name,"connect")==0) || (strcmp(long_name,"connect-port")==0) ||
              (strcmp(long_name,"ca-cert")==0) || (strcmp(long_name,"client-cert")==0) ||
              (strcmp(long_name,"client-key")==0) || (strcmp(long_name,"vsock")==0) )
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if ( (strcmp(long_name,"help")==0) )
         {
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else if ( (strcmp(long_name,"version")==0) )
         {
            m_display_version = true;
            return ARGPARSE_SUCCESS;
         }
         else if ( (strcmp(long_name,"no-bind")==0) )
         {
            m_no_bind = true;
         }
         else if ( (strcmp(long_name,"no-signature")==0) )
         {
            m_no_signature = true;
         }
         else if ( (strcmp(long_name,"tls")==0) )
         {
            m_use_tls = true;
         }
         else
         {
            return ARGPARSE_NAME_ERROR;
         }
      }
   }
   else
   {
      if ( short_name != NULL )
      {
         char *end;
         long lval;
         if (strcmp(short_name,"p")==0)
         {
            lval = strtol(value, &end, 0);
            if ((end > value) && (lval <= UINT16_MAX))
            {
               m_bind_port = (uint16_t) lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name,"r")==0)
         {
            lval = strtol(value, &end, 0);
            if ( (end > value) && (lval <= UINT16_MAX))
            {
               m_connect_port = (uint16_t) lval;
               m_connect_port_set = true;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name,"b")==0)
         {
            if (m_bind_address != NULL) adt_str_delete(m_bind_address);
            m_bind_resource_type = msocket_parse_endpoint(value, &m_bind_address, &m_bind_port);
            if ( (m_bind_resource_type == MSOCKET_ENDPOINT_UNKNOWN) ||
                 (m_bind_resource_type == MSOCKET_ENDPOINT_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name,"c")==0)
         {
            if (m_connect_address != NULL) adt_str_delete(m_connect_address);
            m_connect_resource_type = msocket_parse_endpoint(value, &m_connect_address, &m_connect_port);
            if ( (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN) ||
                 (m_connect_resource_type == MSOCKET_ENDPOINT_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
      }
      else if (long_name != NULL)
      {
         char *end;
         long lval;
         if (strcmp(long_name,"bind-port")==0)
         {
            lval = strtol(value, &end, 0);
            if ( (end > value) && (lval <= UINT16_MAX))
            {
               m_bind_port = (uint16_t) lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name,"connect-port")==0)
         {
            lval = strtol(value, &end, 0);
            if ( (end > value) && (lval <= UINT16_MAX))
            {
               m_connect_port = (uint16_t) lval;
               m_connect_port_set = true;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name,"ca-cert")==0)
         {
            if (m_ca_cert != NULL) adt_str_delete(m_ca_cert);
            m_ca_cert = adt_str_new_cstr(value);
         }
         else if (strcmp(long_name,"client-cert")==0)
         {
            if (m_client_cert != NULL) adt_str_delete(m_client_cert);
            m_client_cert = adt_str_new_cstr(value);
         }
         else if (strcmp(long_name,"client-key")==0)
         {
            if (m_client_key != NULL) adt_str_delete(m_client_key);
            m_client_key = adt_str_new_cstr(value);
         }
         else if (strcmp(long_name,"bind")==0)
         {
            if (m_bind_address != NULL) adt_str_delete(m_bind_address);
            m_bind_resource_type = msocket_parse_endpoint(value, &m_bind_address, &m_bind_port);
            if ( (m_bind_resource_type == MSOCKET_ENDPOINT_UNKNOWN) ||
                 (m_bind_resource_type == MSOCKET_ENDPOINT_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name,"connect")==0)
         {
            if (m_connect_address != NULL) adt_str_delete(m_connect_address);
            m_connect_resource_type = msocket_parse_endpoint(value, &m_connect_address, &m_connect_port);
            if ( (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN) ||
                 (m_connect_resource_type == MSOCKET_ENDPOINT_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name,"vsock")==0)
         {
            char const *colon = strchr(value, ':');
            if (colon == NULL)
            {
               return ARGPARSE_VALUE_ERROR;
            }
            char *end = NULL;
            unsigned long port_val = strtoul(colon + 1, &end, 0);
            if ((end == colon + 1) || (*end != '\0') || (port_val == 0) || (port_val > UINT32_MAX))
            {
               return ARGPARSE_VALUE_ERROR;
            }
            m_vsock_port = (uint32_t) port_val;

            size_t cid_len = (size_t) (colon - value);
            if (cid_len == 0)
            {
               return ARGPARSE_VALUE_ERROR;
            }
            char cid_str[32];
            if (cid_len >= sizeof(cid_str))
            {
               return ARGPARSE_VALUE_ERROR;
            }
            memcpy(cid_str, value, cid_len);
            cid_str[cid_len] = '\0';

            if (strcmp(cid_str, "any") == 0)
            {
               m_vsock_cid = MSOCKET_VMADDR_CID_ANY;
            }
            else if (strcmp(cid_str, "host") == 0)
            {
               m_vsock_cid = MSOCKET_VMADDR_CID_HOST;
            }
            else if (strcmp(cid_str, "local") == 0)
            {
               m_vsock_cid = MSOCKET_VMADDR_CID_LOCAL;
            }
            else if (strcmp(cid_str, "hypervisor") == 0)
            {
               m_vsock_cid = MSOCKET_VMADDR_CID_HYPERVISOR;
            }
            else
            {
               unsigned long cid_val = strtoul(cid_str, &end, 0);
               if ((end == cid_str) || (*end != '\0') || (cid_val > UINT32_MAX))
               {
                  return ARGPARSE_VALUE_ERROR;
               }
               m_vsock_cid = (uint32_t) cid_val;
            }
            m_use_vsock = true;
         }
      }
      else
      {
         adt_str_set_cstr(&m_definition_file, value);
      }
   }
   return ARGPARSE_SUCCESS;
}

/**
 * Reads contents of text file into a string
 */
static adt_str_t *read_definition_file(adt_str_t *path)
{
   adt_bytearray_t *definition_bytes = ifstream_util_readTextFile(adt_str_cstr(path));
   if (definition_bytes == NULL)
   {
      printf("Failed to read text file: %s\n", adt_str_cstr(path));
   }
   else
   {
      adt_str_t *str = adt_str_new_bytearray(definition_bytes);
      adt_bytearray_delete(definition_bytes);
      if (str == NULL)
      {
         printf("Failed to create string from bytearray\n");
      }
      return str;
   }
   return NULL;
}

static void print_version(void)
{
   printf("%s %s\n", APP_NAME, SW_VERSION_LITERAL);
}

static void print_usage(const char *arg0)
{
   printf("%s [-b --bind bind_path] [-p --bind-port port] [--no-bind] "
              "[-c --connect connect_path] [-r --connect-port connect_port] "
              "[--no-signature] "
              "[--tls] [--ca-cert ca_path] [--client-cert cert_path] [--client-key key_path] "
              "[--vsock <cid>:<port>] "
              "[--version] "
              "definition_file\n", arg0);
}

static void application_shutdown(void)
{
   if (m_messageServerRunning)
   {
      printf("Shutting down JSON message server...");
      json_server_shutdown();
      printf("OK\n");
   }
   if (m_apx_connection != NULL)
   {
      printf("Closing APX connection...");
      apx_connection_disconnect(m_apx_connection);
      apx_connection_delete(m_apx_connection);
      printf("OK\n");
   }
}

static void application_cleanup(void)
{
   adt_str_destroy(&m_definition_file);
   if (m_bind_address) adt_str_delete(m_bind_address);
   if (m_connect_address) adt_str_delete(m_connect_address);
   if (m_apx_definition_str != NULL) adt_str_delete(m_apx_definition_str);
   if (m_ca_cert) adt_str_delete(m_ca_cert);
   if (m_client_cert) adt_str_delete(m_client_cert);
   if (m_client_key) adt_str_delete(m_client_key);
}

#ifndef _WIN32
static void signal_handler_setup(void)
{
   if(signal (SIGINT, signal_handler) == SIG_IGN) {
      signal (SIGINT, SIG_IGN);
   }
   if(signal (SIGTERM, signal_handler) == SIG_IGN) {
      signal (SIGTERM, SIG_IGN);
   }
   signal(SIGPIPE, SIG_IGN);
}

static void signal_handler(int signum)
{
   (void)signum;
   m_runFlag = 0;
}
#endif

static apx_error_t connect_to_apx_server(void)
{
   if (m_use_vsock)
   {
      if (m_use_tls)
      {
         printf("Error: TLS over VSOCK is not supported\n");
         return APX_INVALID_ARGUMENT_ERROR;
      }
#ifdef _WIN32
      printf("Error: VSOCK not supported on Windows\n");
      return APX_NOT_IMPLEMENTED_ERROR;
#else
      return apx_connection_connect_vsock(m_apx_connection, m_vsock_cid, m_vsock_port);
#endif
   }
   const char *connect_address = adt_str_cstr(m_connect_address);
#if defined(MSOCKET_ENABLE_TLS)
   if (m_use_tls)
   {
      const char *ip_addr = connect_address;
      if (m_connect_resource_type == MSOCKET_ENDPOINT_NAME)
      {
         if ((strlen(connect_address) == 0) || (strcmp(connect_address, "localhost") == 0))
         {
            ip_addr = "127.0.0.1";
         }
         else
         {
            return APX_INVALID_ARGUMENT_ERROR;
         }
      }
      else if (m_connect_resource_type != MSOCKET_ENDPOINT_IPV4)
      {
         printf("Error: TLS connection requires IPv4 address or localhost\n");
         return APX_INVALID_ARGUMENT_ERROR;
      }

      msocket_tls_config_t tls_config;
      msocket_tls_config_create(&tls_config);
      if (m_ca_cert != NULL)
      {
         msocket_tls_config_set_ca_cert(&tls_config, adt_str_cstr(m_ca_cert));
      }
      else
      {
         if (cutil_file_exists(EXAMPLE_CA_CERT_PATH))
         {
            msocket_tls_config_set_ca_cert(&tls_config, EXAMPLE_CA_CERT_PATH);
         }
         else if (cutil_file_exists(PYTEST_CA_CERT_PATH))
         {
            msocket_tls_config_set_ca_cert(&tls_config, PYTEST_CA_CERT_PATH);
         }
         else
         {
            fprintf(stderr, "Error: No CA certificate found\n");
         }
      }

      if (m_client_cert != NULL && m_client_key != NULL)
      {
         msocket_tls_config_set_client_cert(&tls_config, adt_str_cstr(m_client_cert), adt_str_cstr(m_client_key));
      }
      else if (m_client_cert == NULL && m_client_key == NULL)
      {
         if (cutil_file_exists(EXAMPLE_CLIENT_CERT_PATH) && cutil_file_exists(EXAMPLE_CLIENT_KEY_PATH))
         {
            msocket_tls_config_set_client_cert(&tls_config, EXAMPLE_CLIENT_CERT_PATH, EXAMPLE_CLIENT_KEY_PATH);
         }
         else if (cutil_file_exists(PYTEST_CLIENT_CERT_PATH) && cutil_file_exists(PYTEST_CLIENT_KEY_PATH))
         {
            msocket_tls_config_set_client_cert(&tls_config, PYTEST_CLIENT_CERT_PATH, PYTEST_CLIENT_KEY_PATH);
         }
         else
         {
            fprintf(stderr, "Error: No client certificate found\n");
         }
      }
      apx_error_t rc = apx_connection_connect_tls(m_apx_connection, ip_addr, m_connect_port, &tls_config);
      msocket_tls_config_destroy(&tls_config);
      return rc;
   }
#else
   if (m_use_tls)
   {
      printf("Error: TLS support is not enabled in this build\n");
      return APX_NOT_IMPLEMENTED_ERROR;
   }
#endif

   switch(m_connect_resource_type)
   {
   case MSOCKET_ENDPOINT_UNKNOWN:
      return APX_INVALID_ARGUMENT_ERROR;
   case MSOCKET_ENDPOINT_IPV4:
      return apx_connection_connect_tcp(m_apx_connection, connect_address, m_connect_port);
   case MSOCKET_ENDPOINT_IPV6:
      return APX_NOT_IMPLEMENTED_ERROR;
   case MSOCKET_ENDPOINT_FILE:
#ifdef _WIN32
      printf("UNIX domain sockets not supported in Windows\n");
      return APX_NOT_IMPLEMENTED_ERROR;
#else
      return apx_connection_connect_unix(m_apx_connection, connect_address);
#endif
   case MSOCKET_ENDPOINT_NAME:
      if ( (strlen(connect_address) == 0) || (strcmp(connect_address, "localhost") == 0) )
      {
         return apx_connection_connect_tcp(m_apx_connection, "127.0.0.1", m_connect_port);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t init_json_message_server(void)
{
   uint8_t address_family = 255u;
   const char* bind_address = adt_str_cstr(m_bind_address);
   switch (m_bind_resource_type)
   {
   case MSOCKET_ENDPOINT_IPV4:
      address_family = MSOCKET_ADDR_INET;
      break;
   case MSOCKET_ENDPOINT_IPV6:
      address_family = MSOCKET_ADDR_INET6;
      break;
   case MSOCKET_ENDPOINT_FILE:
#ifdef _WIN32
      printf("UNIX domain sockets not supported in Windows\n");
      return APX_NOT_IMPLEMENTED_ERROR;
#else
      address_family = MSOCKET_ADDR_UNIX;
      break;
#endif
   case MSOCKET_ENDPOINT_NAME:
      if ((strlen(bind_address) == 0) || (strcmp(bind_address, "localhost") == 0))
      {
         address_family = MSOCKET_ADDR_INET;
      }
      break;
   default:
      break;
   }
   if (address_family != 255u)
   {
      return json_server_init(m_apx_connection, address_family);
   }
   return APX_CONNECTION_ERROR;
}

static apx_error_t start_json_message_server(void)
{
   const char *bind_address = adt_str_cstr(m_bind_address);
   switch(m_bind_resource_type)
   {
   case MSOCKET_ENDPOINT_UNKNOWN:
      return APX_INVALID_ARGUMENT_ERROR;
   case MSOCKET_ENDPOINT_IPV4: //fall-through
   case MSOCKET_ENDPOINT_IPV6:
      return json_server_start_tcp(bind_address, m_bind_port);
   case MSOCKET_ENDPOINT_FILE:
#ifdef _WIN32
      printf("UNIX domain sockets not supported in Windows\n");
      return APX_NOT_IMPLEMENTED_ERROR;
#else
      return json_server_start_unix(bind_address);
#endif
   case MSOCKET_ENDPOINT_NAME:
      if ( (strlen(bind_address) == 0) || (strcmp(bind_address, "localhost") == 0) )
      {
         return json_server_start_tcp("127.0.0.1", m_bind_port);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
