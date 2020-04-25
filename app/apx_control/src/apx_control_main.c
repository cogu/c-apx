/*****************************************************************************
* \file      apx_listen_main.c
* \author    Conny Gustafsson
* \date      2020-04-12
* \brief     apx_listen console application
*
* Copyright (c) 2020 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "adt_str.h"
#include "message_client_connection.h"
#include "apx_error.h"
#include "apx_util.h"
#include "argparse.h"
#include "dtl_json.h"
#include "filestream.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
static int init_wsa(void);
#endif
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value);
static void print_usage(const char *arg0);
static void application_cleanup(void);
#ifndef _WIN32
static void connect_and_send_message_unix(const char *socketPath);
#endif
static void connect_and_send_message_tcp(const char *address, uint16_t port, uint8_t addressFamily);
static apx_error_t read_message_from_file(const char *file_path);
static adt_error_t build_json_message(const adt_str_t *name, const adt_str_t *value);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

/*** Argument variables ***/
static const uint16_t connect_port_default = 5100;
#ifdef _WIN32
static const char *m_connect_address_default = "127.0.0.1";
#else
static const char *m_connect_address_default = "/tmp/apx_listen.socket";
#endif
static uint16_t m_pos_arg_count = 0u;
static uint16_t m_connect_port;
static adt_str_t *m_connect_address = (adt_str_t*) 0;
static adt_str_t *m_message = (adt_str_t*) 0;
static adt_str_t *m_input_file_path = (adt_str_t*) 0;
static adt_str_t *m_name = (adt_str_t*) 0;
static adt_str_t *m_value = (adt_str_t*) 0;
static apx_resource_type_t m_connect_resource_type = APX_RESOURCE_TYPE_UNKNOWN;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   m_connect_port = connect_port_default;
   int retval = 0;
   argparse_result_t result = argparse_exec(argc, (const char**) argv, argparse_cbk);
   if (result == ARGPARSE_SUCCESS)
   {
#ifdef _WIN32
      if (init_wsa() != 0)
      {
         int err = WSAGetLastError();
         fprintf(stderr, "WSAStartup failed with error: %d\n", err);
         retval = 1;
         goto SHUTDOWN;
      }
#endif
      if (m_connect_resource_type == APX_RESOURCE_TYPE_UNKNOWN)
      {
         uint16_t dummy_port;
         m_connect_resource_type = apx_parse_resource_name(m_connect_address_default, &m_connect_address, &dummy_port);
         (void) dummy_port;
         assert( (m_connect_resource_type != APX_RESOURCE_TYPE_UNKNOWN) && (m_connect_resource_type != APX_RESOURCE_TYPE_ERROR) );
      }
      if (m_input_file_path != 0)
      {
         apx_error_t result;
         const char *input_file_path = adt_str_cstr(m_input_file_path);
         result = read_message_from_file(input_file_path);

         if (result != APX_NO_ERROR)
         {
            fprintf(stderr, "Error: Failed to read contents of file %s\n", input_file_path);
         }
         else
         {
            dtl_dv_t *dv = dtl_json_load_cstr(adt_str_cstr(m_message));
            if (dv == 0)
            {
               fprintf(stderr, "Error: JSON data validation error while parsing %s\n", input_file_path);
               retval = -1;
               goto SHUTDOWN;
            }
            else
            {
               dtl_dec_ref(dv);
            }
         }
      }
      else
      {
         if (m_name != 0)
         {
            if (m_value == 0)
            {
               printf("Error: missing value argument\n");
               print_usage(argv[0]);
               goto SHUTDOWN;
            }
            else
            {
               adt_error_t result = build_json_message(m_name, m_value);
               if (result != ADT_NO_ERROR)
               {
                  retval = -1;
                  goto SHUTDOWN;
               }
            }
         }
      }
      if ( (m_message != 0) && (m_connect_resource_type != APX_RESOURCE_TYPE_UNKNOWN) && (m_connect_resource_type != APX_RESOURCE_TYPE_ERROR) )
      {
         const char *address;
         address = adt_str_cstr(m_connect_address);
         assert(address != 0);
         switch(m_connect_resource_type)
         {
         case APX_RESOURCE_TYPE_IPV4: //fall-trough
            connect_and_send_message_tcp(address, m_connect_port, AF_INET);
            break;
         case APX_RESOURCE_TYPE_IPV6:
            connect_and_send_message_tcp(address, m_connect_port, AF_INET6);
            break;
         case APX_RESOURCE_TYPE_FILE:
#ifdef _WIN32
            printf("UNIX domain socket path not supported in Windows\n");
#else
            connect_and_send_message_unix(address);
#endif
            break;
         case APX_RESOURCE_TYPE_NAME:
            if ( (strlen(address) == 0) || (strcmp(address, "localhost") == 0) )
            {
               connect_and_send_message_tcp("127.0.0.1", m_connect_port, AF_INET);
            }
            else
            {
               fprintf(stderr, "Error: Unsupported connection name \"%s\"\n", address);
               retval = -1;
            }
            break;
         }
      }
      else if (m_message == 0)
      {
         printf("No message to send\n");
      }
   }
   else
   {
      printf("Error parsing argument (%d)\n", (int) result);
      print_usage(argv[0]);
   }
SHUTDOWN:
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
#ifdef _WIN32
static int init_wsa(void)
{
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;
   wVersionRequested = MAKEWORD(2, 2);
   err = WSAStartup(wVersionRequested, &wsaData);
   return err;
}
#endif
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value)
{
   if (value == 0)
   {
      if ( short_name != 0 )
      {
         if ( (strcmp(short_name,"i")==0) || (strcmp(short_name,"p")==0) ||
              (strcmp(short_name,"c")==0) )
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if( (strcmp(short_name,"h")==0) )
         {
            return ARGPARSE_SUCCESS;
         }
         else
         {
            return ARGPARSE_NAME_ERROR;
         }
      }
      else if ( (long_name != 0) )
      {
         if ( (strcmp(long_name,"connect")==0) || (strcmp(long_name,"port")==0) )
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if ( (strcmp(long_name,"help")==0) )
         {
            return ARGPARSE_SUCCESS;
         }
         else
         {
            return ARGPARSE_NAME_ERROR;
         }
      }
   }
   else
   {
      if ( short_name != 0 )
      {
         char *end;
         long lval;
         if (strcmp(short_name,"p")==0)
         {
            lval = strtol(value, &end, 0);
            if ((end > value) && (lval <= UINT16_MAX))
            {
               m_connect_port = (uint16_t) lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name,"c")==0)
         {
            if (m_connect_address != 0) adt_str_delete(m_connect_address);
            m_connect_resource_type = apx_parse_resource_name(value, &m_connect_address, &m_connect_port);
            if ( (m_connect_resource_type == APX_RESOURCE_TYPE_UNKNOWN) ||
                 (m_connect_resource_type == APX_RESOURCE_TYPE_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name, "i") == 0)
         {
            m_input_file_path = adt_str_new_cstr(value);
            if (m_input_file_path == 0)
            {
               return ARGPARSE_MEM_ERROR;
            }
         }
         else
         {
            return ARGPARSE_PARSE_ERROR;
         }
      }
      else if (long_name != 0)
      {
         char *end;
         long lval;
         if (strcmp(long_name,"port")==0)
         {
            lval = strtol(value, &end, 0);
            if ( (end > value) && (lval <= UINT16_MAX))
            {
               m_connect_port = (uint16_t) lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name,"connect")==0)
         {
            if (m_connect_address != 0) adt_str_delete(m_connect_address);
            m_connect_resource_type = apx_parse_resource_name(value, &m_connect_address, &m_connect_port);
            if ( (m_connect_resource_type == APX_RESOURCE_TYPE_UNKNOWN) ||
                 (m_connect_resource_type == APX_RESOURCE_TYPE_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
      }
      else
      {
         adt_str_t *tmp = adt_str_new_cstr(value);
         if (tmp == 0)
         {
            return ARGPARSE_MEM_ERROR;
         }
         if (m_pos_arg_count==0u)
         {
            m_name = tmp;
         }
         else if (m_pos_arg_count==1u)
         {
            m_value = tmp;
         }
         else
         {
            adt_str_delete(tmp);
            return ARGPARSE_PARSE_ERROR;
         }
         m_pos_arg_count++;
      }
   }
   return ARGPARSE_SUCCESS;
}



static void print_usage(const char *arg0)
{
   printf("%s [-i file_name]\n"
          "[-c --connect connect_path] [-p --port connect_port]\n"
          "[name value]", arg0);
}

static void application_cleanup(void)
{
   if (m_connect_address != 0) adt_str_delete(m_connect_address);
   if (m_message != 0) adt_str_delete(m_message);
   if (m_input_file_path != 0) adt_str_delete(m_input_file_path);
}

#ifndef _WIN32
static void connect_and_send_message_unix(const char *socketPath)
{
   message_client_connection_t *connection = message_client_connection_new(AF_UNIX);
   assert(m_message != 0);
   if (connection != 0)
   {
      adt_error_t rc = message_client_prepare_message(connection, m_message);
      if (rc == ADT_NO_ERROR)
      {
         int32_t result;
         result = message_client_connect_unix(connection, socketPath);
         if (result != 0)
         {
            printf("Failed to connect\n");
         }
         result = message_client_wait_for_message_transmitted(connection);
         if (result != 0)
         {
            printf("message_client_wait_for_message_transmitted failed with %d\n", (int) result);
         }
      }
      else
      {
         printf("Failed to prepare data\n");
      }
      message_client_connection_delete(connection);
   }
}
#endif

static void connect_and_send_message_tcp(const char *address, uint16_t port, uint8_t addressFamily)
{
   if ((addressFamily == AF_INET) || (addressFamily == AF_INET6))
   {
      message_client_connection_t* connection = message_client_connection_new(addressFamily);
      assert(m_message != 0);
      if (connection != 0)
      {
         adt_error_t rc = message_client_prepare_message(connection, m_message);
         if (rc == ADT_NO_ERROR)
         {
            int32_t result;
            result = message_client_connect_tcp(connection, address, port);
            if (result != 0)
            {
               printf("Failed to connect\n");
            }
            result = message_client_wait_for_message_transmitted(connection);
            if (result != 0)
            {
               printf("message_client_wait_for_message_transmitted failed with %d\n", (int)result);
            }
         }
         else
         {
            printf("Failed to prepare data\n");
         }
         message_client_connection_delete(connection);
      }
   }
}

static apx_error_t read_message_from_file(const char *file_path)
{
   apx_error_t retval = APX_NO_ERROR;
   adt_bytearray_t *bytes = ifstream_util_readTextFile(file_path);
   if (bytes != 0)
   {
      m_message = adt_str_new_bytearray(bytes);
      adt_bytearray_delete(bytes);
      if (m_message == 0)
      {
         retval = APX_MEM_ERROR;
      }
   }
   else
   {
      retval = APX_READ_ERROR;
   }
   return retval;
}

static adt_error_t build_json_message(const adt_str_t *name, const adt_str_t *value)
{
   adt_error_t result;
   dtl_dv_t *dv;
   adt_str_t* json_message = adt_str_new();
   result = adt_str_push(json_message, '{');
   if (result != ADT_NO_ERROR) return result;
   result = adt_str_push(json_message, '"');
   if (result != ADT_NO_ERROR) return result;
   result = adt_str_append(json_message, name);
   if (result != ADT_NO_ERROR) return result;
   result = adt_str_push(json_message, '"');
   if (result != ADT_NO_ERROR) return result;
   result = adt_str_push(json_message, ':');
   if (result != ADT_NO_ERROR) return result;
   result = adt_str_append(json_message, value);
   if (result != ADT_NO_ERROR) return result;
   result = adt_str_push(json_message, '}');
   if (result != ADT_NO_ERROR) return result;
   dv = dtl_json_load_cstr(adt_str_cstr(json_message));
   if (dv == 0)
   {
      fprintf(stderr, "Error: Failed to validate JSON data\n");
      return ADT_INVALID_ARGUMENT_ERROR;
   }
   else
   {
      dtl_dec_ref(dv);
      m_message = json_message;
   }
   return ADT_NO_ERROR;
}
