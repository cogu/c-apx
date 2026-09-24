/*****************************************************************************
* \file      server_main.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX server daemon main
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# if defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
# endif
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "apx/server.h"
#include "apx/types.h"
#include "dtl_json.h"
#include "extensions_cfg.h"
#include "server_cfg.h"
#include "apx/event_listener.h"
#include "argparse.h"
#include "apx_version.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx_server"

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value);
static void print_version(void);
static void print_usage(const char *name);
#ifndef _WIN32
static void signal_handler_setup(void);
void signal_handler(int signum);
#endif
#ifdef _WIN32
static int init_wsa(void);
#endif

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
//int8_t g_debug; // Global so apx_logging can use it from everywhere
int m_runFlag = 1;

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_server_t m_server;
static bool m_display_help = false;
static bool m_display_version = false;
static adt_str_t *m_config_path = NULL;
static adt_str_t *m_socket_config = NULL;
static const char *SW_VERSION_STR = SW_VERSION_LITERAL;
static int m_ready_fd = -1;
//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   apx_error_t result;
   dtl_hv_t *config = NULL;

   m_runFlag = 1;
   m_display_help = false;
   m_display_version = false;
   m_config_path = NULL;
   m_ready_fd = -1;

   argparse_result_t parse_result = argparse_exec(argc, (const char**) argv, argparse_cbk);
   if (parse_result != ARGPARSE_SUCCESS)
   {
      print_usage(argv[0]);
      return 1;
   }
   if (m_display_help)
   {
      print_usage(argv[0]);
      return 0;
   }
   if (m_display_version)
   {
      print_version();
      return 0;
   }
   if (m_config_path == NULL && m_socket_config == NULL)
   {
      print_usage(argv[0]);
      return 0;
   }

   printf("APX Server %s\n\n", SW_VERSION_STR);
   if (m_config_path != NULL)
   {
      const char* config_path = adt_str_cstr(m_config_path);
      printf("Loading %s: ", config_path);
      fflush(stdout);
      result = apx_server_load_config(config_path, &config);
      adt_str_delete(m_config_path);
      m_config_path = NULL;
      if (result != APX_NO_ERROR)
      {
         printf("Error %d: %s\n", (int) result, apx_strerror(result));
         if (m_socket_config != NULL)
         {
            adt_str_delete(m_socket_config);
            m_socket_config = NULL;
         }
         return 1;
      }
      else
      {
         printf("OK\n");
         fflush(stdout);
      }
   }
   else
   {
      config = dtl_hv_new();
      if (config == NULL)
      {
         fprintf(stderr, "Failed to allocate configuration hash\n");
         if (m_socket_config != NULL)
         {
            adt_str_delete(m_socket_config);
            m_socket_config = NULL;
         }
         return 1;
      }
   }

   if (m_socket_config != NULL)
   {
      dtl_dv_t *socket_dv = dtl_json_load_cstr(adt_str_cstr(m_socket_config));
      adt_str_delete(m_socket_config);
      m_socket_config = NULL;
      if (socket_dv == NULL || dtl_dv_type(socket_dv) != DTL_DV_HASH)
      {
         fprintf(stderr, "Error: --socket-config must be a valid JSON object\n");
         if (socket_dv != NULL)
         {
            dtl_dec_ref(socket_dv);
         }
         if (config != NULL)
         {
            dtl_dec_ref(config);
         }
         return 1;
      }
      dtl_hv_set_cstr(config, "socket-server-extension", socket_dv, false);
   }

#ifndef _WIN32
   signal_handler_setup();
#endif
   apx_server_create(&m_server);
   result = register_apx_server_extensions(&m_server, config);
   if (result != APX_NO_ERROR)
   {
      fprintf(stderr, "Failed to register server extensions: %s (error %d)\n", apx_strerror(result), (int) result);
      apx_server_destroy(&m_server);
      if (config != NULL)
      {
         dtl_dec_ref(config);
      }
      return 1;
   }
   apx_server_start(&m_server);
#ifndef _WIN32
   if (m_ready_fd >= 0)
   {
      char ready_byte = '\n';
      if (write(m_ready_fd, &ready_byte, 1) < 0)
      {
         perror("write ready_fd");
      }
      close(m_ready_fd);
      m_ready_fd = -1;
   }
#endif
   while(m_runFlag != 0)
   {
      SLEEP(1000); //main thread is sleeping while child threads do all the work
   }
   printf("Server shutdown started\n");
   apx_server_destroy(&m_server);
   if (config != NULL)
   {
      dtl_dec_ref(config);
   }
   printf("Server shutdown complete\n");
#if defined(_MSC_VER) && defined(_DEBUG)
   _CrtDumpMemoryLeaks();
#endif
   return 0;
}

#ifdef MEM_LEAK_CHECK
void vfree(void *arg)
{
   free(arg);
}
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

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

void signal_handler(int signum)
{
   (void)signum;
   m_runFlag = false;
}
#endif

static void print_version(void)
{
   printf("%s %s\n", APP_NAME, SW_VERSION_LITERAL);
}

static void print_usage(const char *name)
{
   printf("Usage:\n%s [-h | --help] [--version] [-r <fd> | --ready-fd <fd>] [-s <json> | --socket-config <json>] [<config_file | config_dir>]\n", name);
}

static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value)
{
   if (short_name != NULL)
   {
      if (strcmp(short_name, "h") == 0)
      {
         m_display_help = true;
         return ARGPARSE_SUCCESS;
      }
      if (strcmp(short_name, "r") == 0)
      {
         if (value == NULL)
         {
            return ARGPARSE_NEED_VALUE;
         }
         char *endptr = NULL;
         long fd = strtol(value, &endptr, 10);
         if (endptr == value || *endptr != '\0' || fd < 0)
         {
            return ARGPARSE_VALUE_ERROR;
         }
         m_ready_fd = (int) fd;
         return ARGPARSE_SUCCESS;
      }
      if (strcmp(short_name, "s") == 0)
      {
         if (value == NULL)
         {
            return ARGPARSE_NEED_VALUE;
         }
         m_socket_config = adt_str_new_cstr(value);
         return ARGPARSE_SUCCESS;
      }
      return ARGPARSE_NAME_ERROR;
   }
   if (long_name != NULL)
   {
      if (strcmp(long_name, "help") == 0)
      {
         m_display_help = true;
         return ARGPARSE_SUCCESS;
      }
      if (strcmp(long_name, "version") == 0)
      {
         m_display_version = true;
         return ARGPARSE_SUCCESS;
      }
      if (strcmp(long_name, "ready-fd") == 0)
      {
         if (value == NULL)
         {
            return ARGPARSE_NEED_VALUE;
         }
         char *endptr = NULL;
         long fd = strtol(value, &endptr, 10);
         if (endptr == value || *endptr != '\0' || fd < 0)
         {
            return ARGPARSE_VALUE_ERROR;
         }
         m_ready_fd = (int) fd;
         return ARGPARSE_SUCCESS;
      }
      if (strcmp(long_name, "socket-config") == 0)
      {
         if (value == NULL)
         {
            return ARGPARSE_NEED_VALUE;
         }
         m_socket_config = adt_str_new_cstr(value);
         return ARGPARSE_SUCCESS;
      }
      return ARGPARSE_NAME_ERROR;
   }
   if (value != NULL)
   {
      m_config_path = adt_str_new_cstr(value);
      return ARGPARSE_SUCCESS;
   }
   return ARGPARSE_PARSE_ERROR;
}


