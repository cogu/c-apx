/*****************************************************************************
* \file      apx_listener_main.c
* \author    Conny Gustafsson
* \date      2020-03-09
* \brief     apx_listener application
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
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include "apx_receive_connection.h"
#include "filestream.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_SERVER_PATH "/tmp/apx_server.socket"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void print_usage(char *name);
static void signal_handler_setup(void);
void signal_handler(int signum);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
int m_runFlag = 1;
apx_receive_connection_t *m_apx_connection;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   adt_bytearray_t *definition_bytes = (adt_bytearray_t*) 0;
   adt_str_t *definition_str = (adt_str_t*) 0;
   apx_error_t rc;
   sigset_t mask, oldmask;
   apx_nodeInstance_t *nodeInstance;
   apx_portCount_t numRequirePorts = 0;
   const char *apx_server_path_default = APX_SERVER_PATH;
   signal_handler_setup();
   if (argc < 2)
   {
      print_usage(argv[0]);
      return 0;
   }
   else
   {
      const char *file_name = argv[1];
      definition_bytes = ifstream_util_readTextFile(file_name);
      if (definition_bytes == 0)
      {
         printf("Failed to read text file: %s\n", file_name);
         return 1;
      }
      else
      {
         definition_str = adt_str_new_bytearray(definition_bytes);
         adt_bytearray_delete(definition_bytes);
         definition_bytes = (adt_bytearray_t*) 0;
         if (definition_str == 0)
         {
            printf("Failed to create string from bytearray\n");
            return 1;
         }
      }
   }
   sigemptyset(&mask);
   sigaddset(&mask, SIGINT);
   sigaddset(&mask, SIGTERM);
   sigprocmask(SIG_BLOCK, &mask, &oldmask);

   printf("Creating APX client...");
   m_apx_connection = apx_receive_connection_new();
   if (m_apx_connection == 0)
   {
      printf("Failed\n");
      return 1;
   }
   else
   {
      printf("OK\n");
   }
   printf("Parsing definition...");
   rc = apx_receive_connection_attachNode(m_apx_connection, definition_str);
   if (rc != APX_NO_ERROR)
   {
      if (rc == APX_PARSE_ERROR)
      {
         int32_t errorLine = apx_receive_connection_getLastErrorLine(m_apx_connection);
         printf("Parse error on line %d\n", (int) errorLine);
      }
      else
      {
         printf("Failed with error %d\n", (int) rc);
      }
      return 1;
   }
   else
   {
      printf("OK\n");
   }

   nodeInstance = apx_receive_connection_getLastAttachedNode(m_apx_connection);
   if (nodeInstance != 0)
   {
      numRequirePorts = apx_nodeInstance_getNumRequirePorts(nodeInstance);
   }

   if (definition_str != 0)
   {
      adt_str_delete(definition_str);
      definition_str = (adt_str_t*) 0;
   }

   printf("Connecting to APX server...");
   rc = apx_receive_connection_connect_unix(m_apx_connection, apx_server_path_default);
   if (rc != APX_NO_ERROR)
   {
      printf("Failed with error code %d\n", (int) rc);
      return 1;
   }
   else
   {
      printf("OK\n");
   }

   printf("Ready (%d ports)\n", (int) numRequirePorts);
   while(m_runFlag)
   {
      sigsuspend(&oldmask);
   }
   sigprocmask(SIG_UNBLOCK, &mask, NULL);
   printf("Shutting down APX client...");
   apx_receive_connection_delete(m_apx_connection);
   printf("OK\n");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void print_usage(char *name)
{
   printf("Usage:\n%s apx_definition_file\n",name);
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

void signal_handler(int signum)
{
   (void)signum;
   m_runFlag = 0;
}
