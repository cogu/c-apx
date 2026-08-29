#define CLEANUP_TEST 1                  //0=no cleanup test (default), 1=enable cleanup test
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#if CLEANUP_TEST
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#include <WinSock2.h>
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
#ifdef USE_CONFIGURATION_FILE
#include "apx_build_cfg.h"
#endif

#pragma comment(lib, "ws2_32.lib")
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx_server"
#define SHUTDOWN_TIMER_WARN_THRESHOLD 10
#if CLEANUP_TEST
#define SHUTDOWN_TIMER_INIT 10     //number of seconds before server shutdown is triggered in a cleanup test
#else
#define SHUTDOWN_TIMER_INIT 0
#endif

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
static int32_t m_shutdownTimer;
static bool m_display_help = false;
static bool m_display_version = false;
static adt_str_t *m_config_path = NULL;
static const char *SW_VERSION_STR = SW_VERSION_LITERAL;
//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   apx_error_t result;
   dtl_hv_t *server_config = NULL;
   dtl_hv_t *extensions_config = NULL;

   m_shutdownTimer = SHUTDOWN_TIMER_INIT;
   m_runFlag = 1;
   m_display_help = false;
   m_display_version = false;
   m_config_path = NULL;

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
   if (m_config_path == NULL)
   {
      print_usage(argv[0]);
      return 0;
   }

   const char* config_path = adt_str_cstr(m_config_path);
   printf("APX Server %s\n\n", SW_VERSION_STR);
   printf("Loading %s: ", config_path);
   fflush(stdout);
   result = apx_server_load_config(config_path, &server_config, &extensions_config);
   adt_str_delete(m_config_path);
   if (result != APX_NO_ERROR)
   {
      printf("Error %d: %s\n", (int) result, apx_strerror(result));
      return 1;
   }
   else
   {
      printf("OK\n");
      fflush(stdout);
      if (server_config != NULL)
      {
         dtl_sv_t *svShutdownTimer = (dtl_sv_t*) dtl_hv_get_cstr(server_config, "shutdown-timer");
         if (svShutdownTimer != NULL)
         {
            int32_t i32;
            bool ok;
            i32 = dtl_sv_to_i32(svShutdownTimer, &ok);
            if (ok)
            {
               m_shutdownTimer = i32;
            }
         }
      }
   }

#ifdef _WIN32
   if (init_wsa() != 0)
   {
      int err = WSAGetLastError();
      fprintf(stderr, "WSAStartup failed with error: %d\n", err);
      return 1;
   }
#else
   signal_handler_setup();
#endif
   apx_server_create(&m_server);
   result = register_apx_server_extensions(&m_server, extensions_config);
   if (result != APX_NO_ERROR)
   {
      fprintf(stderr, "Failed to register server extensions: %s (error %d)\n", apx_strerror(result), (int) result);
      apx_server_destroy(&m_server);
      if (server_config != NULL)
      {
         dtl_dec_ref(server_config);
      }
      if (extensions_config != NULL)
      {
         dtl_dec_ref(extensions_config);
      }
#ifdef _WIN32
      WSACleanup();
#endif
      return 1;
   }
   apx_server_start(&m_server);
   while(m_runFlag != 0)
   {
      SLEEP(1000); //main thread is sleeping while child threads do all the work
      if (m_shutdownTimer > 0)
      {
         if (--m_shutdownTimer==0) //this counter is used during a cleanup test to verify that all resources are properly cleaned up
         {
            break;
         }
         if (m_shutdownTimer < SHUTDOWN_TIMER_WARN_THRESHOLD)
         {
            char msg[40];
            sprintf(msg, "Shutdown in %ds", m_shutdownTimer);
            apx_server_log_write(&m_server, APX_LOG_LEVEL_INFO, "main", msg);
         }
      }
   }
   printf("Server shutdown started\n");
   apx_server_destroy(&m_server);
   if (server_config != NULL)
   {
      dtl_dec_ref(server_config);
   }
   if (extensions_config != NULL)
   {
      dtl_dec_ref(extensions_config);
   }
   printf("Server shutdown complete\n");   
#ifdef _WIN32
   WSACleanup();
#endif
#if defined(_MSC_VER) && (CLEANUP_TEST != 0)
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
   printf("Usage:\n%s [-h | --help] [--version] <config_dir>\n", name);
}

static argparse_result_t argparse_cbk(const char *short_name, const char *long_name, const char *value)
{
   if (value == NULL)
   {
      if (short_name != NULL)
      {
         if (strcmp(short_name, "h") == 0)
         {
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else
         {
            return ARGPARSE_NAME_ERROR;
         }
      }
      else if (long_name != NULL)
      {
         if (strcmp(long_name, "help") == 0)
         {
            m_display_help = true;
            return ARGPARSE_SUCCESS;
         }
         else if (strcmp(long_name, "version") == 0)
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
      if (short_name == NULL && long_name == NULL)
      {
         m_config_path = adt_str_new_cstr(value);
      }
      else
      {
         return ARGPARSE_NAME_ERROR;
      }
   }
   return ARGPARSE_SUCCESS;
}

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
