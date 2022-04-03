/*****************************************************************************
* \file      apx_perf_test_main.c
* \author    Conny Gustafsson
* \date      2019-10-13
* \brief     APX performance test application
*
* Copyright (c) 2019-2022 Conny Gustafsson
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "msocket.h"
#include "application.h"
#include "osmacro.h"
#include "filestream.h"
#include "adt_str.h"
#include "argparse.h"
#include "apx/util.h"
#ifdef USE_CONFIGURATION_FILE
#include "apx_build_cfg.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx_perf"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
static int init_wsa(void);
#endif
static argparse_result_t argparse_cbk(const char* short_name, const char* long_name, const char* value);
static void print_version(void);
static void print_usage(const char* arg0);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
/*** Argument variables ***/
static const uint16_t connect_port_default = 5000u;
#ifdef _WIN32
static const char* m_connect_address_default = "127.0.0.1";
#else
static const char* m_connect_address_default = "/tmp/apx_server.socket";
#endif
static const uint32_t m_timer_default = 5u;
static bool m_display_help = false;
static bool m_display_version = false;
static bool m_is_requester = true;
static uint16_t m_pos_arg_count = 0u;
static uint16_t m_connect_port;
static adt_str_t* m_connect_address = (adt_str_t*)0;
static adt_str_t* m_message = (adt_str_t*)0;
static adt_str_t* m_input_file_path = (adt_str_t*)0;
static adt_str_t* m_name = (adt_str_t*)0;
static adt_str_t* m_value = (adt_str_t*)0;
static apx_resource_type_t m_connect_resource_type = APX_RESOURCE_TYPE_UNKNOWN;
uint32_t m_timer_init = 0u;

static const char* requester_apx_def =
"APX/1.3\n"
"N\"RequestNode\"\n"
"P\"PerfTest_rqst\"L:=0xFFFFFFFF\n"
"R\"PerfTest_rsp\"L:=0xFFFFFFFF\n";
static const char* responder_apx_def =
"APX/1.3\n"
"N\"RespondNode\"\n"
"R\"PerfTest_rqst\"L:=0xFFFFFFFF\n"
"P\"PerfTest_rsp\"L:=0xFFFFFFFF\n";



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   int retval = 0;
   application_cfg_t cfg;
   m_connect_port = connect_port_default;
   m_timer_init = m_timer_default;
   argparse_result_t result = argparse_exec(argc, (const char**)argv, argparse_cbk);
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
         (void)dummy_port;
         assert((m_connect_resource_type != APX_RESOURCE_TYPE_UNKNOWN) && (m_connect_resource_type != APX_RESOURCE_TYPE_ERROR));
      }
      memset(&cfg, 0, sizeof(cfg));
      cfg.apx_definition = m_is_requester ? requester_apx_def : responder_apx_def;
      cfg.resource_type = m_connect_resource_type;
      cfg.server_address = adt_str_cstr(m_connect_address);
      cfg.tcp_port = m_connect_port;
      cfg.timer_init = m_timer_init;
      if (!application_init(&cfg))
      {
         retval = -1;
         goto SHUTDOWN;
      }
      for (;;)
      {
         SLEEP(1000);
         if (!application_run())
         {
            break;
         }
      }
   }
SHUTDOWN:
   return retval;
}

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

static argparse_result_t argparse_cbk(const char* short_name, const char* long_name, const char* value)
{
   if (value == 0)
   {
      if (short_name != 0)
      {
         if ((strcmp(short_name, "t") == 0) || (strcmp(short_name, "p") == 0) || (strcmp(short_name, "c") == 0))
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if ((strcmp(short_name, "h") == 0))
         {
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else if ((strcmp(short_name, "s") == 0))
         {
            m_is_requester = false;
            return ARGPARSE_SUCCESS;
         }
         else
         {
            return ARGPARSE_NAME_ERROR;
         }
      }
      else if ((long_name != 0))
      {
         if ((strcmp(long_name, "connect") == 0) || (strcmp(long_name, "port") == 0) || (strcmp(long_name, "time") == 0))
         {
            return ARGPARSE_NEED_VALUE;
         }
         else if ((strcmp(long_name, "help") == 0))
         {
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else if ((strcmp(long_name, "version") == 0))
         {
            m_display_version = true;
            return ARGPARSE_SUCCESS;
         }
         else if ((strcmp(long_name, "slave") == 0))
         {
            m_is_requester = false;
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
      if (short_name != 0)
      {
         char* end = NULL;
         long lval;
         if (strcmp(short_name, "p") == 0)
         {
            lval = strtol(value, &end, 0);
            if ((end > value) && (lval <= UINT16_MAX))
            {
               m_connect_port = (uint16_t)lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name, "t") == 0)
         {
            lval = strtol(value, &end, 0);
            if ((end > value) && (lval <= UINT16_MAX))
            {
               m_timer_init = (uint16_t)lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(short_name, "c") == 0)
         {
            if (m_connect_address != 0) adt_str_delete(m_connect_address);
            m_connect_resource_type = apx_parse_resource_name(value, &m_connect_address, &m_connect_port);
            if ((m_connect_resource_type == APX_RESOURCE_TYPE_UNKNOWN) ||
               (m_connect_resource_type == APX_RESOURCE_TYPE_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else
         {
            return ARGPARSE_PARSE_ERROR;
         }
      }
      else if (long_name != 0)
      {
         char* end = NULL;
         long lval;
         if (strcmp(long_name, "port") == 0)
         {
            lval = strtol(value, &end, 0);
            if ((end > value) && (lval <= UINT16_MAX))
            {
               m_connect_port = (uint16_t)lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name, "connect") == 0)
         {
            if (m_connect_address != 0) adt_str_delete(m_connect_address);
            m_connect_resource_type = apx_parse_resource_name(value, &m_connect_address, &m_connect_port);
            if ((m_connect_resource_type == APX_RESOURCE_TYPE_UNKNOWN) ||
               (m_connect_resource_type == APX_RESOURCE_TYPE_ERROR))
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
         else if (strcmp(long_name, "time") == 0)
         {
            lval = strtol(value, &end, 0);
            if ((end > value) && (lval <= UINT16_MAX))
            {
               m_timer_init = (uint16_t)lval;
            }
            else
            {
               return ARGPARSE_VALUE_ERROR;
            }
         }
      }
      else
      {
         adt_str_t* tmp = adt_str_new_cstr(value);
         if (tmp == 0)
         {
            return ARGPARSE_MEM_ERROR;
         }
         if (m_pos_arg_count == 0u)
         {
            m_name = tmp;
         }
         else if (m_pos_arg_count == 1u)
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

static void print_version(void)
{
   printf("%s %s\n", APP_NAME, SW_VERSION_LITERAL);
}

static void print_usage(const char* arg0)
{
   printf("%s "
      "[-c --connect connect_path] [-p --port connect_port] "
      "[--version] "
      "[-s --slave] "
      "[-t --time seconds]\n"
      , arg0);
}