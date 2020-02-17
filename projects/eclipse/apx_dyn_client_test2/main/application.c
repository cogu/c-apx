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
/*
static void onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg);
static void inPortDataWrittenCbk(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);
*/

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_client_t *m_client = NULL;
static apx_nodeInstance_t *m_nodeInstance = NULL;
static uint16_t m_wheelBasedVehicleSpeed;
static uint32_t m_stressCount;
static bool m_isConnected;
static bool m_hasPendingStressCmd;
static bool m_isStressOngoing;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void application_init(const char *apx_definition, const char *unix_socket_path)
{
   apx_clientEventListener_t handlerTable;
   apx_error_t result;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.clientConnect2 = onClientConnected;
   handlerTable.clientDisconnect2 = onClientDisconnected;
   //handlerTable.logEvent = onLogEvent;

   m_wheelBasedVehicleSpeed = 0u;
   m_stressCount = 0;
   m_isConnected = false;
   m_hasPendingStressCmd = true;
   m_isStressOngoing = false;

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
      m_nodeInstance = apx_client_getLastAttachedNode(m_client);
      assert(m_nodeInstance != 0);

      uint8_t data[UINT16_SIZE];
      packLE(&data[0], m_wheelBasedVehicleSpeed, UINT16_SIZE);
      apx_nodeInstance_writeProvidePortData(m_nodeInstance, &data[0], 1, UINT16_SIZE);

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
      uint8_t data[UINT16_SIZE];
      m_wheelBasedVehicleSpeed++;
      packLE(&data[0], m_wheelBasedVehicleSpeed, UINT16_SIZE);
      apx_nodeInstance_writeProvidePortData(m_nodeInstance, &data[0], 1, UINT16_SIZE);
   }
   else
   {
      printf("Not connected\n");
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
   m_hasPendingStressCmd = true;
   m_isStressOngoing = false;
   printf("Client disconnected from server\n");
}
/*
static void onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg)
{
   printf("onLogEvent\n");
}

static void inPortDataWrittenCbk(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   if ( m_isTestNode1 && m_isStressOngoing && (offset == 0) && (len == UINT32_SIZE))
   {
      m_stressCount++;
      uint8_t tmp[UINT32_SIZE] = {0, 0, 0, 0};
      apx_nodeData_readInPortData(m_nodeData, &tmp[0], offset, len);
      packLE(&tmp[0], m_stressCount, UINT32_SIZE);
      apx_nodeData_updateOutPortData(m_nodeData, &tmp[0], 3, UINT32_SIZE, false);

   }
   else if( (offset == 3) && (len == UINT32_SIZE))
   {
      uint8_t tmp[UINT32_SIZE] = {0, 0, 0, 0};
      apx_nodeData_readInPortData(m_nodeData, &tmp[0], offset, len);
      (void) unpackLE(&tmp[0], UINT32_SIZE);
      apx_nodeData_updateOutPortData(m_nodeData, &tmp[0], 0, UINT32_SIZE, false);
   }
}
*/
