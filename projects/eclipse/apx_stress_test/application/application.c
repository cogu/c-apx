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
#define APX_SOCKET_PATH "/tmp/apx_server.socket"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void onClientConnected(void *arg, apx_clientConnectionBase_t *clientConnection);
static void onClientDisconnected(void *arg, apx_clientConnectionBase_t *clientConnection);
static void onRequirePortWrite(void *arg, struct apx_nodeInstance_tag *nodeInstance, apx_portId_t requirePortId, void *portHandle);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_client_t *m_client = NULL;
static apx_nodeInstance_t *m_nodeInstance = NULL;
static uint32_t m_stressCount;
static bool m_isConnected;
static bool m_hasPendingStressCmd;
static bool m_isStressOngoing;
static bool m_isSendNode;
static void *m_rqst_handle;
static void *m_rsp_handle;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void application_init(const char *apx_definition)
{
   apx_clientEventListener_t handlerTable;
   apx_error_t result;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.clientConnect1 = onClientConnected;
   handlerTable.clientDisconnect1 = onClientDisconnected;
   handlerTable.requirePortWrite1 = onRequirePortWrite;

   m_isSendNode = false;
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
      if ( (strcmp(apx_nodeInstance_getName(m_nodeInstance), "SendNode")==0) )
      {
         m_isSendNode = true;
      }
      m_rqst_handle = apx_client_getPortHandle(m_client, NULL, "StressTest_rqst");
      m_rsp_handle = apx_client_getPortHandle(m_client, NULL, "StressTest_rsp");
      assert(m_rqst_handle != 0);
      assert(m_rsp_handle != 0);
      result = apx_client_connect_unix(m_client, APX_SOCKET_PATH);
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
      if (m_isSendNode)
      {
         if (m_hasPendingStressCmd)
         {
            apx_error_t result;
            m_stressCount = 0u;
            m_hasPendingStressCmd = false;
            m_isStressOngoing = true;
            result = apx_client_writePortData_u32(m_client, m_rqst_handle, m_stressCount);
            assert(result == APX_NO_ERROR);
            printf("Stress test started\n");
         }
         else if (m_isStressOngoing)
         {
            printf("Stress test completed, count=%d\n", m_stressCount);
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

   m_isConnected = true;

}

static void onClientDisconnected(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   m_isConnected = false;
   m_hasPendingStressCmd = true;
   m_isStressOngoing = false;
}


static void onRequirePortWrite(void *arg, struct apx_nodeInstance_tag *nodeInstance, apx_portId_t requirePortId, void *portHandle)
{
   if ( (m_isSendNode) && (m_isStressOngoing) && (portHandle == m_rsp_handle))
   {
      apx_error_t result;
      m_stressCount++;
      result = apx_client_writePortData_u32(m_client, m_rqst_handle, m_stressCount);
      assert(result == APX_NO_ERROR);
   }
   else if( (!m_isSendNode) && (portHandle == m_rqst_handle))
   {
      apx_error_t result;
      uint32_t value = 0u;
      result = apx_client_readPortData_u32(m_client, m_rqst_handle, &value);
      assert(result == APX_NO_ERROR);
      result = apx_client_writePortData_u32(m_client, m_rsp_handle, value);
      assert(result == APX_NO_ERROR);
   }
}
