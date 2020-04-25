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
#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include "adt_str.h"
#include "apx_connection.h"
#include "apx_util.h"
#include "argparse.h"
#include "json_server.h"
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
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value);
static adt_str_t *read_definition_file(adt_str_t *path);
static void print_usage(const char *arg0);
static void application_shutdown(void);
static void application_cleanup(void);
static void signal_handler_setup(void);
static void signal_handler(int signum);
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
#ifdef _WIN32
static const char *m_bind_address_default = "127.0.0.1";
static const char *m_connect_address_default = "127.0.0.1";
#else
static const char *m_bind_address_default = "/tmp/apx_listen.socket";
static const char *m_connect_address_default = "/tmp/apx_server.socket";
#endif
static bool m_no_bind = false;
static uint16_t m_bind_port;
static uint16_t m_connect_port;
static adt_str_t *m_bind_address = (adt_str_t*) 0;
static adt_str_t *m_connect_address = (adt_str_t*) 0;
static adt_str_t m_definition_file;
static apx_resource_type_t m_bind_resource_type = APX_RESOURCE_TYPE_UNKNOWN;
static apx_resource_type_t m_connect_resource_type = APX_RESOURCE_TYPE_UNKNOWN;

/*** Other local variables***/
static adt_str_t *m_apx_definition_str = (adt_str_t*) 0;
static apx_connection_t *m_apx_connection = (apx_connection_t*) 0;
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
      if (m_bind_resource_type == APX_RESOURCE_TYPE_UNKNOWN)
      {
         uint16_t dummy_port;
         m_bind_resource_type = apx_parse_resource_name(m_bind_address_default, &m_bind_address, &dummy_port);
         (void) dummy_port;
         assert( (m_bind_resource_type != APX_RESOURCE_TYPE_UNKNOWN) && (m_bind_resource_type != APX_RESOURCE_TYPE_ERROR) );
      }
      if (m_connect_resource_type == APX_RESOURCE_TYPE_UNKNOWN)
      {
         uint16_t dummy_port;
         m_connect_resource_type = apx_parse_resource_name(m_connect_address_default, &m_connect_address, &dummy_port);
         (void) dummy_port;
         assert( (m_connect_resource_type != APX_RESOURCE_TYPE_UNKNOWN) && (m_connect_resource_type != APX_RESOURCE_TYPE_ERROR) );
      }
      if (adt_str_length(&m_definition_file) == 0)
      {
         printf("Error: No definition file given\n");
         print_usage(argv[0]);
      }
      else
      {
         printf("Initializing APX connection...");
         m_apx_connection = apx_connection_new();
         if (m_apx_connection != 0)
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
         if (m_apx_definition_str != 0)
         {
            printf("Parsing %s (%d bytes)...", adt_str_cstr(&m_definition_file), adt_str_size(m_apx_definition_str));
            apx_error_t rc = apx_connection_attachNode(m_apx_connection, m_apx_definition_str);
            if (rc != APX_NO_ERROR)
            {
               if (rc == APX_PARSE_ERROR)
               {
                  int32_t errorLine = apx_connection_getLastErrorLine(m_apx_connection);
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
               apx_nodeInstance_t *nodeInstance;
               apx_portCount_t numProvidePorts;
               apx_portCount_t numRequirePorts;
               printf("OK\n");
               nodeInstance = apx_connection_getLastAttachedNode(m_apx_connection);
               if (nodeInstance != 0)
               {
                  numProvidePorts = apx_nodeInstance_getNumProvidePorts(nodeInstance);
                  numRequirePorts = apx_nodeInstance_getNumRequirePorts(nodeInstance);
                  printf("\t%s: Provide-Ports: %d, Require-Ports: %d\n",
                        apx_nodeInstance_getName(nodeInstance),
                        (int) numProvidePorts, (int) numRequirePorts);
               }
               printf("Connecting to APX server at %s...", adt_str_cstr(m_connect_address));
               rc = connect_to_apx_server();
               if (rc == APX_NO_ERROR)
               {
                  sigset_t mask, oldmask;
                  printf("OK\n");
                  if (!m_no_bind)
                  {
                     apx_error_t rc;
                     printf("Initializing JSON message server...");
                     rc = json_server_init(m_apx_connection);
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
               }
               else
               {
                  printf("Failed (%d)\n", (int) rc);
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
   if (value == 0)
   {
      if ( short_name != 0 )
      {
         if ( (strcmp(short_name,"b")==0) || (strcmp(short_name,"p")==0) ||
              (strcmp(short_name,"c")==0) || (strcmp(short_name,"r")==0) )
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
         if ( (strcmp(long_name,"bind")==0) || (strcmp(long_name,"bind-port")==0) ||
              (strcmp(long_name,"connect")==0) || (strcmp(long_name,"connect-port")==0) )
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if ( (strcmp(long_name,"help")==0) )
         {
            return ARGPARSE_SUCCESS;
         }
         else if ( (strcmp(long_name,"no-bind")==0) )
         {
            m_no_bind=true;
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
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name,"b")==0)
         {
            if (m_bind_address != 0) adt_str_delete(m_bind_address);
            m_bind_resource_type = apx_parse_resource_name(value, &m_bind_address, &m_bind_port);
            if ( (m_bind_resource_type == APX_RESOURCE_TYPE_UNKNOWN) ||
                 (m_bind_resource_type == APX_RESOURCE_TYPE_ERROR))
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
      }
      else if (long_name != 0)
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
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name,"bind")==0)
         {
            if (m_bind_address != 0) adt_str_delete(m_bind_address);
            m_bind_resource_type = apx_parse_resource_name(value, &m_bind_address, &m_bind_port);
            if ( (m_bind_resource_type == APX_RESOURCE_TYPE_UNKNOWN) ||
                 (m_bind_resource_type == APX_RESOURCE_TYPE_ERROR))
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
   if (definition_bytes == 0)
   {
      printf("Failed to read text file: %s\n", adt_str_cstr(path));
   }
   else
   {
      adt_str_t *str = adt_str_new_bytearray(definition_bytes);
      adt_bytearray_delete(definition_bytes);
      if (str == 0)
      {
         printf("Failed to create string from bytearray\n");
      }
      return str;
   }
   return (adt_str_t*) 0;
}

static void print_usage(const char *arg0)
{
   printf("%s [-b --bind bind_path] [-p --bind-port port]\n"
              "[-c --connect connect_path] [-r --connect-port connect_port]\n"
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
   if (m_apx_connection != 0)
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
   if (m_apx_definition_str != 0) adt_str_delete(m_apx_definition_str);
}

static void signal_handler_setup(void)
{
   if(signal (SIGINT, signal_handler) == SIG_IGN) {
      signal (SIGINT, SIG_IGN);
   }
   if(signal (SIGTERM, signal_handler) == SIG_IGN) {
      signal (SIGTERM, SIG_IGN);
   }
}

static void signal_handler(int signum)
{
   (void)signum;
   m_runFlag = 0;
}

static apx_error_t connect_to_apx_server(void)
{
   const char *connect_address = adt_str_cstr(m_connect_address);
   switch(m_connect_resource_type)
   {
   case APX_RESOURCE_TYPE_UNKNOWN:
      return APX_INVALID_ARGUMENT_ERROR;
   case APX_RESOURCE_TYPE_IPV4:
      return apx_connection_connect_tcp(m_apx_connection, connect_address, m_connect_port);
   case APX_RESOURCE_TYPE_IPV6:
      return APX_NOT_IMPLEMENTED_ERROR;
   case APX_RESOURCE_TYPE_FILE:
#ifdef _WIN32
      printf("UNIX domain sockets not supported in Windows\n");
      return APX_NOT_IMPLEMENTED_ERROR;
#else
      return apx_connection_connect_unix(m_apx_connection, connect_address);
#endif
   case APX_RESOURCE_TYPE_NAME:
      if ( (strlen(connect_address) == 0) || (strcmp(connect_address, "localhost") == 0) )
      {
         return apx_connection_connect_tcp(m_apx_connection, "127.0.0.1", m_connect_port);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t start_json_message_server(void)
{
   const char *bind_address = adt_str_cstr(m_bind_address);
   switch(m_bind_resource_type)
   {
   case APX_RESOURCE_TYPE_UNKNOWN:
      return APX_INVALID_ARGUMENT_ERROR;
   case APX_RESOURCE_TYPE_IPV4:
      return json_server_start_tcp(bind_address, m_bind_port);
   case APX_RESOURCE_TYPE_IPV6:
      return APX_NOT_IMPLEMENTED_ERROR;
   case APX_RESOURCE_TYPE_FILE:
#ifdef _WIN32
      printf("UNIX domain sockets not supported in Windows\n");
      return APX_NOT_IMPLEMENTED_ERROR;
#else
      return json_server_start_unix(bind_address);
#endif
   case APX_RESOURCE_TYPE_NAME:
      if ( (strlen(bind_address) == 0) || (strcmp(bind_address, "localhost") == 0) )
      {
         return json_server_start_tcp("127.0.0.1", m_bind_port);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
