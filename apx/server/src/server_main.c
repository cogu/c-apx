
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include "apx_server.h"
#include "apx_types.h"
#include "apx_logging.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define DEFAULT_PORT 5000

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int parse_args(int argc, char **argv);
static void printUsage(char *name);
static void main_signal_handler(int signum);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
int8_t g_debug; // Global so apx_logging can use it from everywhere

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static int8_t m_running = 1;
static uint16_t m_port;
static apx_server_t m_server;
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
   g_debug = 0;
   m_port = DEFAULT_PORT;
   printf("APX Server %s\n", SW_VERSION_STR);
   if(argc>1)
   {
      int result = parse_args(argc, argv);
      if (result != 0)
      {
         return 0;
      }
   }
   if(m_port==0)
   {
      printUsage(argv[0]);
      return 0;
   }
   APX_LOG_INFO("Listening on port %d\n", (int)m_port);
#ifdef _WIN32
   wVersionRequested = MAKEWORD(2, 2);
   err = WSAStartup(wVersionRequested, &wsaData);
   if (err != 0) {
      /* Tell the user that we could not find a usable Winsock DLL*/
      APX_LOG_ERROR("WSAStartup failed with error: %d\n", err);
      return 1;
   }
#endif
   apx_server_create(&m_server,m_port);
   apx_server_setDebugMode(&m_server, g_debug);
   apx_server_start(&m_server);

   signal(SIGINT, main_signal_handler);
   signal(SIGTERM, main_signal_handler);
   for(;;)
   {
      //main thread is sleeping while child threads do all the work
#ifdef _WIN32
      SLEEP(5000);
      if(m_running == 0)
      {
         break;
      }
#else
      pause();
      break;
#endif
   }
   APX_LOG_INFO("destroying server\n");
   apx_server_destroy(&m_server);
#ifdef _WIN32
   WSACleanup();
#endif
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void main_signal_handler(int signum)
{
   APX_LOG_INFO("signal caught %d\n", signum);
   m_running = 0;
}

static int parse_args(int argc, char **argv)
{
   int i;
   for(i=1;i<argc;i++)
   {
      if (strncmp(argv[i],"-p=",3)==0)
      {
         char *endptr=0;
         long num = strtol(&argv[i][3],&endptr,10);
         if (endptr > &argv[i][3])
         {
            m_port=(int)num;
         }
      }
      else if (strncmp(argv[i],"-p",2)==0)
      {
         char *endptr=0;
         long num = strtol(&argv[i][2],&endptr,10);
         if (endptr > &argv[i][2])
         {
            m_port=(int)num;
         }
      }      
      else if (strncmp(argv[i], "-h", 2) == 0)
      {
         printUsage(argv[0]);
         return -1;
      }
      else if (strncmp(argv[i], "--debug=", 8) == 0)
      {
         char *endptr=0;
         long num = strtol(&argv[i][8],&endptr,10);
         if (endptr > &argv[i][8])
         {
            g_debug=(int8_t) num;
            if (g_debug == APX_DEBUG_1_PROFILE)
            {
               // Disable stdout buffer to get accurate log timestamps
               setvbuf(stdout, NULL, _IONBF, 0);
            }
         }
      }
      else
      {
         printf("Unknown argument %s\n", argv[i]);
         printUsage(argv[0]);
         return -1;
      }
   }
   return 0;
}

static void printUsage(char *name)
{   
   printf("%s -p<port> [--debug=<level 1-4>]\n",name);
}


