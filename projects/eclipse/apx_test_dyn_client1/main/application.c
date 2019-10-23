/*****************************************************************************
* \file      application.c
* \author    Conny Gustafsson
* \date      2019-10-13
* \brief     Description
*
* Copyright (c) 2019 Conny Gustafsson
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
static void onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg);
static void inPortDataWrittenCbk(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_client_t *m_client = NULL;
static apx_nodeData_t *m_nodeData = NULL;
static uint16_t m_wheelBasedVehicleSpeed;
static uint32_t m_stressCount;
static bool m_isConnected;
static bool m_hasPendingStressCmd;
static bool m_isStressOngoing;
static bool m_isTestNode1;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void application_init(const char *apx_definition, const char *tcp_addr, uint16_t tcp_port)
{
   apx_clientEventListener_t handlerTable;
   apx_error_t result;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.clientConnected = onClientConnected;
   handlerTable.clientDisconnected = onClientDisconnected;
   handlerTable.logEvent = onLogEvent;

   m_wheelBasedVehicleSpeed = 0u;
   m_stressCount = 0;
   m_isConnected = false;
   m_hasPendingStressCmd = true;
   m_isStressOngoing = false;

   m_client = apx_client_new();
   if (m_client != 0)
   {
      apx_client_registerEventListener(m_client, &handlerTable);
      result = apx_client_createLocalNode_cstr(m_client, apx_definition);
      if (result != APX_NO_ERROR)
      {
         printf("apx_client_createLocalNode_cstr returned %d\n", (int) result);
         return;
      }
      m_nodeData = apx_client_getDynamicNode(m_client, 0);
      if ( (strcmp(apx_nodeData_getName(m_nodeData), "TestNode1")==0) )
      {
         m_isTestNode1 = true;
      }
      else
      {
         m_isTestNode1 = false;
      }
      //result = apx_client_connectTcp(m_client, tcp_addr, tcp_port);
      result = apx_client_connectUnix(m_client, "/tmp/apx_server.socket");
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
      if (m_isTestNode1)
      {
         if (m_hasPendingStressCmd)
         {
            uint8_t tmp[UINT32_SIZE] = {0, 0, 0, 0};
            m_hasPendingStressCmd = false;
            m_isStressOngoing = true;
            apx_nodeData_updateOutPortData(m_nodeData, &tmp[0], 3, UINT32_SIZE, false);
            printf("Stress started\n");
         }
         else if (m_isStressOngoing)
         {
            printf("Stress completed, count=%d\n", m_stressCount);
            m_isStressOngoing = false;
         }
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void onClientConnected(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   apx_nodeDataEventListener_t eventListener;
   m_isConnected = true;
   memset(&eventListener, 0, sizeof(eventListener));
   eventListener.inPortDataWritten = inPortDataWrittenCbk;
   apx_clientConnectionBase_registerNodeDataEventListener(clientConnection, &eventListener);
}

static void onClientDisconnected(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   m_isConnected = false;
   m_hasPendingStressCmd = true;
   m_isStressOngoing = false;
}

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
