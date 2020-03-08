/*****************************************************************************
* \file      main.c
* \author    Conny Gustafsson
* \date      2020-03-07
* \brief     apx_control is used to send new messages to the apx_sender process
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "osmacro.h"
#include "filestream.h"
#include "dtl_json.h"
#include "adt_str.h"
#include "message_client_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define socket_path "/tmp/apx_sender.socket"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void print_usage(void);
static void connect_and_send_message_unix(const char *socketPath, adt_str_t *str);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
   dtl_dv_t *dv = 0;
   if (argc == 1)
   {
      dv = dtl_json_load(stdin);
      if (dv == 0)
      {
         printf("Failed to parse JSON from stdin\n");
         return 1;
      }
   }
   else if (argc == 3)
   {
      if (strcmp(argv[1], "-i")==0)
      {
         const char *file_name = argv[2];
         FILE *fh = fopen(file_name, "r");
         if (fh != 0)
         {
            dv = dtl_json_load(fh);
            fclose(fh);
         }
         if (dv == 0)
         {
            printf("Failed to open file \"%s\"\n", file_name);
            return 1;
         }
      }
      else if (strcmp(argv[1], "-h")==0)
      {
         print_usage();
      }
      else
      {
         adt_error_t result;
         const char *signal_name = argv[1];
         const char *signal_data = argv[2];
         adt_str_t* json = adt_str_new();
         dtl_dv_t *value = dtl_json_load_cstr(signal_data);
         if (value == 0)
         {
            printf("Failed to parse JSON from %s\n", signal_data);
            return 1;
         }
         dtl_dv_dec_ref(value);
         result = adt_str_push(json, '{');
         assert(result == ADT_NO_ERROR);
         result = adt_str_push(json, '"');
         assert(result == ADT_NO_ERROR);
         result = adt_str_append_cstr(json, signal_name);
         assert(result == ADT_NO_ERROR);
         result = adt_str_push(json, '"');
         assert(result == ADT_NO_ERROR);
         result = adt_str_push(json, ':');
         assert(result == ADT_NO_ERROR);
         result = adt_str_push(json, ' ');
         assert(result == ADT_NO_ERROR);
         result = adt_str_append_cstr(json, signal_data);
         assert(result == ADT_NO_ERROR);
         result = adt_str_push(json, ' ');
         assert(result == ADT_NO_ERROR);
         result = adt_str_push(json, '}');

         dv = dtl_json_load_cstr(adt_str_cstr(json));
         adt_str_delete(json);
         if (dv == 0)
         {
            printf("Failed to parse JSON from second argument\n");
            return 1;
         }
      }
   }
   else
   {
      print_usage();
   }
   if (dv != 0)
   {
      adt_str_t *str = dtl_json_dumps(dv, 0, false);
      assert(str != 0);
      connect_and_send_message_unix(socket_path, str);
      adt_str_delete(str);
      dtl_dec_ref(dv);
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void print_usage(void)
{
   printf("apx_control:\n"
         "  Mode 1: apx_control (no arguments)\n"
         "          Reads JSON data from stdin\n"
         "  Mode 2: apx_control name value\n"
         "          name of signal followed by its data (JSON formatted)\n"
         "  Mode 3: apx_control -i file_name\n"
         "          Reads JSON data from file\n"
         );
}

static void connect_and_send_message_unix(const char *socketPath, adt_str_t *message)
{
   message_client_connection_t *connection = message_client_connection_new(AF_UNIX);
   if (connection != 0)
   {
      adt_error_t rc = message_client_prepare_message(connection, message);
      if (rc == ADT_NO_ERROR)
      {
         int32_t result;
         result = message_client_connect_unix(connection, socketPath);
         if (result != 0)
         {
            printf("Failed to connect\n");
         }
         result = message_client_wait_for_message_transmitted(connection);
         if (result != 0)
         {
            printf("message_client_wait_for_message_transmitted failed with %d\n", (int) result);
         }
         usleep(10000u);
      }
      else
      {
         printf("Failed to prepare data\n");
      }
      message_client_connection_delete(connection);
   }
}
