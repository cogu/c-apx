#define CLEANUP_TEST 1                  //0=no cleanup test (default), 1=enable cleanup test
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
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
#include "apx_server.h"
#include "apx_types.h"

#include "apx_eventListener.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SHUTDOWN_TIMER_WARN_THRESHOLD 10
#if CLEANUP_TEST
#define SHUTDOWN_TIMER_INIT 20     //number of seconds before server shutdown is triggered in a cleanup test
#else
#define SHUTDOWN_TIMER_INIT 0
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void signal_handler_setup(void);
void signal_handler(int signum);
static void printUsage(char *name);

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
#ifdef _WIN32
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;
#endif
   if (argc < 2u)
   {
      printUsage(argv[0]);
      return 0;
   }
   m_shutdownTimer = SHUTDOWN_TIMER_INIT;
   g_debug = 0;
   m_runFlag = 1;
   printf("APX Server %s\n", SW_VERSION_STR);
#ifdef _WIN32
   wVersionRequested = MAKEWORD(2, 2);
   err = WSAStartup(wVersionRequested, &wsaData);
   if (err != 0) {
      /* Tell the user that we could not find a usable Winsock DLL*/
      fprintf(stderr, "WSAStartup failed with error: %d\n", err);
      return 1;
   }
#endif
   signal_handler_setup();
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
            printf("Shutdown in %ds\n", m_shutdownTimer);
         }
      }
   }
   printf("Server shutdown started\n");
   apx_server_destroy(&m_server);
   printf("Server shutdown complete\n");
//   apx_eventRecorderSrvTxt_delete(eventRecorderSrvTxt);
#ifdef _WIN32
   WSACleanup();
#endif
#if defined(_MSC_VER) && (CLEANUP_TEST != 0)
   _CrtDumpMemoryLeaks();
#endif
   return 0;
}


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
