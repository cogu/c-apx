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
#include <Windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "apx_server.h"
#include "apx_types.h"
#include "dtl_json.h"
#include "extensions.h"
#include "apx_eventListener.h"
#ifdef USE_CONFIGURATION_FILE
#include "apx_build_cfg.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SHUTDOWN_TIMER_WARN_THRESHOLD 10
#if CLEANUP_TEST
#define SHUTDOWN_TIMER_INIT 10     //number of seconds before server shutdown is triggered in a cleanup test
#else
#define SHUTDOWN_TIMER_INIT 0
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void signal_handler_setup(void);
void signal_handler(int signum);
static void printUsage(char *name);
static apx_error_t load_config_file(const char *filename, dtl_hv_t **hv);
static apx_error_t register_extensions(apx_server_t *server, dtl_hv_t *config);
#ifdef _WIN32
static void init_wsa(void);
#endif

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
int8_t g_debug; // Global so apx_logging can use it from everywhere
int m_runFlag = 1;

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_server_t m_server;
static int32_t m_shutdownTimer;
static const char *SW_VERSION_STR = SW_VERSION_LITERAL;
//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   apx_error_t result;
   dtl_hv_t *server_config = (dtl_hv_t*) 0;

   m_shutdownTimer = SHUTDOWN_TIMER_INIT;
   g_debug = 0;
   m_runFlag = 1;

   if (argc < 2u)
   {
      printUsage(argv[0]);
      return 0;
   }
   printf("APX Server %s\n\n", SW_VERSION_STR);
   result = load_config_file(argv[1], &server_config);
   printf("Loading %s: ", argv[1]);
   if (result != APX_NO_ERROR)
   {
      printf("Error %d\n", (int) result);
      return 1;
   }
   else
   {
      printf("OK\n");
      dtl_dv_t *tmp = dtl_hv_get_cstr(server_config, "server");
      if ( (tmp != 0) && (dtl_dv_type(tmp) == DTL_DV_HASH) )
      {
         int32_t i32;
         bool ok;
         dtl_hv_t *serverCfg = (dtl_hv_t*) tmp;
         dtl_sv_t *svShutdownTimer = (dtl_sv_t*) dtl_hv_get_cstr(serverCfg, "shutdown-timer");
         if (svShutdownTimer != 0)
         {
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
      fprintf(stderr, "WSAStartup failed with error: %d\n", err);
      return 1;
   }
#endif
   signal_handler_setup();
   apx_server_create(&m_server);
   if (server_config != 0)
   {
      dtl_dv_t *extension_config = (dtl_dv_t*) 0;
      extension_config = dtl_hv_get_cstr(server_config, "extension");
      if ( (extension_config != 0) && (dtl_dv_type(extension_config) == DTL_DV_HASH) )
      {
         register_extensions(&m_server, (dtl_hv_t*) extension_config);
      }
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
            apx_server_logEvent(&m_server, APX_LOG_LEVEL_INFO, "main", msg);
         }
      }
   }
   printf("Server shutdown started\n");
   apx_server_destroy(&m_server);
   dtl_dec_ref(server_config);
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


static void printUsage(char *name)
{   
   printf("Usage:\n%s configFile.json\n",name);
}

static apx_error_t load_config_file(const char *filename, dtl_hv_t **hv)
{
   if ( (filename != 0) && (hv != 0) )
   {
      FILE *fh = fopen(filename, "r");
      if (fh != 0)
      {
         dtl_dv_t *json_data = dtl_json_load(fh);
         fclose(fh);
         if (json_data != 0)
         {
            if (dtl_dv_type(json_data) == DTL_DV_HASH)
            {
               *hv = (dtl_hv_t*) json_data;
            }
            else
            {
               dtl_dec_ref(json_data);
               return APX_DV_TYPE_ERROR;
            }
            return APX_NO_ERROR;
         }
         else
         {
            return APX_PARSE_ERROR;
         }
      }
      return APX_FILE_NOT_FOUND_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t register_extensions(apx_server_t *server, dtl_hv_t *config)
{
   apx_error_t result;
 /*  result = apx_serverTextLogExtension_register(server, dtl_hv_get_cstr(config, APX_SERVER_TEXTLOG_CFG_KEY));
   if (result != APX_NO_ERROR)
   {
      return result;
   }
*/
   result = apx_socketServerExtension_register(server, dtl_hv_get_cstr(config, APX_SOCKET_SERVER_EXT_CFG_KEY));
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   return APX_NO_ERROR;
}

#ifdef _WIN32
static int init_wsa(void);
{
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;
   wVersionRequested = MAKEWORD(2, 2);
   err = WSAStartup(wVersionRequested, &wsaData);
   return err;
}
#endif

