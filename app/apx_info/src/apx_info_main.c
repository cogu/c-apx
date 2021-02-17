/*****************************************************************************
* \file      apx_info_main.c
* \author    Conny Gustafsson
* \date      2021-02-14
* \brief     apx_info console application
*
* Copyright (c) 2021 Conny Gustafsson
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
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#else
#include <Windows.h>
#include <winsock2.h>
#endif
#include <assert.h>
#include "adt_str.h"
#include "apx_observer_connection.h"
#include "apx_app_cmd.h"
#include "apx/util.h"
#include "argparse.h"
//#include "filestream.h"
#ifdef USE_CONFIGURATION_FILE
#include "apx_build_cfg.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx_info"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value);
static bool parse_command(const char* arg);
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
static apx_error_t connect_to_apx_server(void);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

/*** Argument variables ***/
static const uint16_t connect_port_default = 5000;
#ifdef _WIN32
static const char *m_connect_address_default = "127.0.0.1";
#else
static const char *m_connect_address_default = "/tmp/apx_server.socket";
#endif
static bool m_display_help = false;
static bool m_display_version = false;
static apx_app_cmd_t m_command = APX_APP_CMD_NONE;
static uint16_t m_connect_port;
static adt_str_t *m_connect_address = (adt_str_t*) 0;
static apx_resource_type_t m_connect_resource_type = APX_RESOURCE_TYPE_UNKNOWN;

/*** Other local variables***/
static apx_connection_t *m_apx_connection = (apx_connection_t*) 0;
static int m_runFlag = 1;
static bool m_messageServerRunning = false;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{      
   m_connect_port = connect_port_default;   
   int retval = 0;
   if (argc < 2)
   {
      print_usage(argv[0]);
      return 0;
   }
   if (!parse_command(argv[1]))
   {
      printf("%s: %s is not a valid commmand\n", APP_NAME, argv[1]);
      return 1;
   }
   switch(m_command)
   {
   case APX_APP_CMD_CLIENTS:
      printf("Running clients\n");
      break;
   default:
      printf("Unhandled command: %d\n", m_command);
      return 1;
   }
#if 0
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
      if (m_display_version)
      {
         print_version();
      }
      if (m_display_help)
      {
         print_usage(argv[0]);
      }
/*
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
               }
            }
         }
         else
         {
            fprintf(stderr, "Error: Could not read file '%s'\n", adt_str_cstr(&m_definition_file));
            retval = 1;
            goto SHUTDOWN;
         }         
      }*/
   }
   else
   {
      printf("Error parsing argument (%d)\n", (int) result);
      print_usage(argv[0]);
   }
SHUTDOWN:
   application_shutdown();
   application_cleanup();
#endif
   return retval;
}

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
            m_display_help = true;
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
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else if ( (strcmp(long_name,"version")==0) )
         {
            m_display_version = true;
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
         if (strcmp(short_name,"c")==0)
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
         if (strcmp(long_name,"connect")==0)
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
      }
   }
   return ARGPARSE_SUCCESS;
}

static bool parse_command(const char* arg)
{
   if (strcmp(arg, "clients") == 0)
   {
      m_command = APX_APP_CMD_CLIENTS;
   }
   else
   {
      return false;
   }
   return true;
}

static void print_version(void)
{
   printf("%s %s\n", APP_NAME, SW_VERSION_LITERAL);
}

static void print_usage(const char *arg0)
{
   printf("%s [-b --bind bind_path] [-p --bind-port port] [--no-bind] "
              "[-c --connect connect_path] [-r --connect-port connect_port] "
              "[--version] "
              "definition_file\n", arg0);
}

static void application_shutdown(void)
{
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
   if (m_connect_address) adt_str_delete(m_connect_address);   
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
}

static void signal_handler(int signum)
{
   (void)signum;
   m_runFlag = 0;
}
#else
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

