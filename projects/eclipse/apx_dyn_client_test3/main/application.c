/*****************************************************************************
* \file      application.c
* \author    Conny Gustafsson
* \date      2019-10-13
* \brief     Description
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "apx_eventListener.h"
#include "apx_nodeData.h"
#include "application.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void onClientConnected(void *arg, apx_clientConnectionBase_t *clientConnection);
static void onClientDisconnected(void *arg, apx_clientConnectionBase_t *clientConnection);
static void onClientRequirePortWrite(void *arg, const char *nodeName, apx_portId_t requirePortId, void *portHandle);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_client_t *m_client = NULL;
static bool m_isConnected;
static void *m_vehicleSpeedHandle;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void application_init(const char *apx_definition, const char *unix_socket_path)
{
   apx_clientEventListener_t handlerTable;
   apx_error_t result;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.clientConnect1 = onClientConnected;
   handlerTable.clientDisconnect1 = onClientDisconnected;
   handlerTable.requirePortWrite1 = onClientRequirePortWrite;

   m_isConnected = false;

   m_client = apx_client_new();
   if (m_client != 0)
   {
      apx_client_registerEventListener(m_client, &handlerTable);
      result = apx_client_buildNode_cstr(m_client, apx_definition);
      if (result != APX_NO_ERROR)
      {
         printf("apx_client_createLocalNode_cstr returned %d\n", (int) result);
         return;
      }
      m_vehicleSpeedHandle = apx_client_getPortHandle(m_client, NULL, "VehicleSpeed");
      assert(m_vehicleSpeedHandle != 0);

      result = apx_client_connectUnix(m_client, unix_socket_path);
      if (result != APX_NO_ERROR)
      {
         printf("apx_client_connectTcp returned %d\n", (int) result);
         return;
      }
   }
}

void application_run(void)
{
   if (m_isConnected)
   {

   }
   else
   {

   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void onClientConnected(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   m_isConnected = true;
   printf("Client connected to server\n");
}

static void onClientDisconnected(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   m_isConnected = false;
   printf("Client disconnected from server\n");
}

static void onClientRequirePortWrite(void *arg, const char *nodeName, apx_portId_t requirePortId, void *portHandle)
{
   if (portHandle == m_vehicleSpeedHandle)
   {
      uint16_t u16Value;
      apx_error_t result = apx_client_readPortData_u16(m_client, portHandle, &u16Value);
      if (result == APX_NO_ERROR)
      {
         printf("VehicleSpeed: %d\n", u16Value);
      }
      else
      {
         printf("apx_client_readPortData_u16 failed with %d\n", (int) result);
      }
   }
}
