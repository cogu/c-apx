
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include "apx_pipeConnection.h"
#include "npipe_server.h"
#include "optparse.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define PROG_NAME "apx_pipe"
#define strdup _strdup
#define SYSTEM_PORT 1024

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static bool parse_flags(char* flags);
static bool parse_opt(char *name, char* val);
static void printUsage(char *name);
static void pipeServer_onAccept(void *arg, struct npipe_server_tag *srv, struct npipe_tag *npipe);


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
//options
static uint16_t m_port = 5000;
static const char* m_address_default = "127.0.0.1";
static const char* m_pipe_path_default = "\\\\.\\pipe\\apx";
static char *m_address;
static char *m_pipe_path;


static npipe_server_t m_server;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
   bool result;
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;
   npipe_handlerTable_t handlerTable;
   
   m_pipe_path = strdup(m_pipe_path_default);
   m_address = strdup(m_address_default);
   result = optparse(argc, argv, parse_flags, parse_opt);
   
   if ( result == false )
   {      
      return 1;
   }

   wVersionRequested = MAKEWORD(2, 2);
   err = WSAStartup(wVersionRequested, &wsaData);
   if (err != 0) 
   {
      /* Tell the user that we could not find a usable Winsock DLL*/
      printf("WSAStartup failed with error: %d\n", err);
      return 1;
   }
   npipe_server_create(&m_server, apx_pipeConnection_vdelete);
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.onAccept = pipeServer_onAccept;
   npipe_server_sethandlerTable(&m_server, &handlerTable);   
   npipe_server_start(&m_server, m_pipe_path);
   for (;;)
   {
      Sleep(5000); //main thread sleeps, waiting for ctrl-c signal
   }
   
   WSACleanup();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void printUsage(char *name)
{
   printf("%s [options]\n", name);
   printf("Acts as APX gateway between named pipe connection and tcp connection.\n");
   printf("   --help: Displays this message\n");
   printf("   --pipe: Named pipe server path. Default: \\\\.\\pipe\\apx\n");
   printf("   --port: TCP port of apx server connection. Default: 5000\n");
   printf("   --address: Host address of apx server. Default: 127.0.0.1\n");
}

static bool parse_flags(char* flags)
{
   char *p = flags;
   while(*p != 0)
   {
      printf("flag: %c\n", *p);      
      ++p;
   }
   return true;
}

static bool parse_opt(char *name, char* val)
{
   if (strcmp(name, "help") == 0)
   {
      printUsage(PROG_NAME);
      return false;
   }
   else if (strcmp(name, "pipe") == 0)
   {
      if (val != 0)
      {
         free(m_pipe_path);
         m_pipe_path = strdup(val);
      }
   }
   else if (strcmp(name, "address") == 0)
   {
      if (val != 0)
      {
         free(m_address);
         m_address = strdup(val);
      }
   }
   else if (strcmp(name, "port") == 0)
   {
      if (val != 0)
      {         
         char *end;
         m_port = (uint16_t) strtol(val, &end, 0);
      }
   }
   else
   {
      printf("Unknown option: %s\n", name);
      printUsage(PROG_NAME);
   }
   
   return true;
}

static void pipeServer_onAccept(void *arg, struct npipe_server_tag *srv, struct npipe_tag *npipe)
{   
   if ((m_port >= SYSTEM_PORT) && (m_address != NULL))
   {
      apx_pipeConnection_t *connection = apx_pipeConnection_new(npipe, srv, m_address, m_port);
      if (connection != NULL)
      {
         printf("new connection established\n");         
      }
   }   
}