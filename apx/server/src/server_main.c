
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include "apx_server.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void parse_args(int argc, char **argv);
static void printUsage(char *name);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static uint16_t m_port;
static apx_server_t m_server;
static int32_t m_count;
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
   m_count = 0;
   if(argc<2)
   {
      printUsage(argv[0]);
      return 0;
   }
   parse_args(argc, argv);
   if ( (m_port<0) )
   {
      printUsage(argv[0]);
      return 0;
   }
#ifdef _WIN32
   wVersionRequested = MAKEWORD(2, 2);
   err = WSAStartup(wVersionRequested, &wsaData);
   if (err != 0) {
      /* Tell the user that we could not find a usable Winsock DLL*/
      printf("WSAStartup failed with error: %d\n", err);
      return 1;
   }
#endif
   apx_server_create(&m_server,m_port);
   apx_server_start(&m_server);
   for(;;)
   {
      SLEEP(5000); //main thread is sleeping while child threads do all the work
/*    if (++m_count==20) //this counter is used during testing to verify that all resources are properly cleaned up
      {
         break;
      }*/
   }
   printf("destroying server\n");
   apx_server_destroy(&m_server);
#ifdef _WIN32
   WSACleanup();
#endif
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void parse_args(int argc, char **argv)
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
   }
}

static void printUsage(char *name)
{
   printf("%s -p<port>\n",name);
}


