/*****************************************************************************
* \file      client.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx/client.h"
#include "apx/client_internal.h"
#include "apx/client_connection_base.h"
#include "apx/client_socket_connection.h"
#include "apx/node_manager.h"
#include "apx/file_manager.h"
#include "apx/parser.h"
#include "apx/node_instance.h"
#include "apx/vm.h"
#include "msocket.h"
#include "adt_ary.h"
#include "adt_list.h"
#include "adt_hash.h"
#include "apx/event_listener.h"
#include "apx/compiler.h"
#include "pack.h"
#ifdef UNIT_TEST
#include "testsocket.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_STACK_BUFFER_SIZE 256u
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_client_triggerConnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection);
static void apx_client_triggerDisconnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection);
static void apx_client_triggerRequirePortDataWriteEventOnListeners(apx_client_t *self, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle);
static void apx_client_attachLocalNodesToConnection(apx_client_t *self);
static apx_error_t apx_client_verifySingleInstructionProgramFromPortRef(apx_portRef_t *portRef, uint8_t opcode, uint8_t variant);
static apx_error_t apx_client_verifySingleInstructionProgram(const adt_bytes_t *program, uint8_t opcode, uint8_t variant);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_client_create(apx_client_t *self)
{
   if( self != 0 )
   {
      self->eventListeners = adt_list_new(apx_clientEventListener_vdelete);
      if (self->eventListeners == 0)
      {
         return APX_MEM_ERROR;
      }
      self->connection = (apx_clientConnectionBase_t*) 0;
      self->vm = (apx_vm_t*) 0;
      //The node manager in this class is the true manager of the nodeInstances. Therefore we set useWeakRef argument to false.
      self->nodeManager = apx_nodeManager_new(APX_CLIENT_MODE, false);
      self->isConnected = false;
      SPINLOCK_INIT(self->lock);
      SPINLOCK_INIT(self->eventListenerLock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_client_destroy(apx_client_t *self)
{
   if (self != 0)
   {
      bool isConnected;
      SPINLOCK_ENTER(self->lock);
      isConnected = self->isConnected;
      SPINLOCK_LEAVE(self->lock);
      if (isConnected)
      {
         apx_client_disconnect(self);
      }
      adt_list_delete(self->eventListeners);
      if (self->connection != 0)
      {
         apx_connectionBase_delete(&self->connection->base);
      }
      if (self->nodeManager != 0)
      {
         apx_nodeManager_delete(self->nodeManager);
      }
      if (self->vm != 0)
      {
         apx_vm_delete(self->vm);
      }
      SPINLOCK_DESTROY(self->lock);
      SPINLOCK_DESTROY(self->eventListenerLock);
   }
}

apx_client_t DLL_PUBLIC *apx_client_new(void)
{
   apx_client_t *self = (apx_client_t*) malloc(sizeof(apx_client_t));
   if(self != 0)
   {
      int8_t result = apx_client_create(self);
      if (result != 0)
      {
         free(self);
         self=0;
      }
   }
   return self;
}

void DLL_PUBLIC apx_client_delete(apx_client_t *self)
{
   if (self != 0)
   {
      apx_client_destroy(self);
      free(self);
   }
}

void DLL_PUBLIC apx_client_vdelete(void *arg)
{
   apx_client_delete((apx_client_t*) arg);
}

#ifdef UNIT_TEST
apx_error_t apx_client_connect_testsocket(apx_client_t *self, struct testsocket_tag *socketObject)
{
   if (self != 0)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new(socketObject);
      if (socketConnection)
      {
         apx_error_t result;
         apx_client_attachConnection(self, &socketConnection->base);
         result = apx_clientSocketConnection_connect(socketConnection);
         if (result == APX_NO_ERROR)
         {
            SPINLOCK_ENTER(self->lock);
            self->isConnected = true;
            SPINLOCK_LEAVE(self->lock);
            testsocket_onConnect(socketObject);
         }
         return result;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#else

/**
 * On connection error, the user can retreive the actual error using errno on Linux and WSAGetLastError on Windows
 */
apx_error_t apx_client_connect_tcp(apx_client_t *self, const char *address, uint16_t port)
{
   if (self != 0)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new((msocket_t*) 0);
      if (socketConnection != 0)
      {
         apx_error_t result;
         apx_client_attachConnection(self, (apx_clientConnectionBase_t*) socketConnection);
         result = apx_clientConnection_tcp_connect(socketConnection, address, port);
         if (result == APX_NO_ERROR)
         {
            SPINLOCK_ENTER(self->lock);
            self->isConnected = true;
            SPINLOCK_LEAVE(self->lock);
         }
         return result;
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

# ifndef _WIN32
apx_error_t apx_client_connect_unix(apx_client_t *self, const char *socketPath)
{
   if (self != 0)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new((msocket_t*) 0);
      if (socketConnection != 0)
      {
         apx_error_t result;
         apx_client_attachConnection(self, (apx_clientConnectionBase_t*) socketConnection);
         result = apx_clientConnection_unix_connect(socketConnection, socketPath);
         if (result == APX_NO_ERROR)
         {
            SPINLOCK_ENTER(self->lock);
            self->isConnected = true;
            SPINLOCK_LEAVE(self->lock);
         }
         return result;
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
# endif

#endif

void apx_client_disconnect(apx_client_t *self)
{
   if ( (self != 0) && (self->connection != 0))
   {
      apx_connectionBase_close(&self->connection->base);
      apx_connectionBase_stop(&self->connection->base);
      SPINLOCK_ENTER(self->lock);
      self->isConnected = false;
      SPINLOCK_LEAVE(self->lock);
   }
}


void* apx_client_registerEventListener(apx_client_t *self, struct apx_clientEventListener_tag *listener)
{
   if ( (self != 0) && (listener != 0))
   {
      void *handle = (void*) apx_clientEventListener_clone(listener);
      if (handle != 0)
      {
         SPINLOCK_ENTER(self->eventListenerLock);
         adt_list_insert(self->eventListeners, handle);
         SPINLOCK_LEAVE(self->eventListenerLock);
      }
      return handle;
   }
   return (void*) 0;
}


void apx_client_unregisterEventListener(apx_client_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0) )
   {
      bool deleteSuccess = false;
      SPINLOCK_ENTER(self->eventListenerLock);
      deleteSuccess = adt_list_remove(self->eventListeners, handle);
      SPINLOCK_LEAVE(self->eventListenerLock);
      if (deleteSuccess)
      {
         apx_clientEventListener_vdelete(handle);
      }
   }
}

int32_t apx_client_getNumAttachedNodes(apx_client_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = apx_nodeManager_length(self->nodeManager);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_client_getNumEventListeners(apx_client_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->eventListenerLock);
      retval = adt_list_length(self->eventListeners);
      SPINLOCK_LEAVE(self->eventListenerLock);
      return retval;
   }
   return -1;
}

void apx_client_attachConnection(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      self->connection = connection;
      if (connection->client != self)
      {
         connection->client = self;
      }
      apx_client_attachLocalNodesToConnection(self);
   }
}

apx_clientConnectionBase_t *apx_client_getConnection(apx_client_t *self)
{
   if (self != 0)
   {
      return self->connection;
   }
   return (apx_clientConnectionBase_t*) 0;
}

apx_error_t apx_client_buildNode_cstr(apx_client_t *self, const char *definition_text)
{
   if (self != 0 && definition_text != 0)
   {
      return apx_nodeManager_buildNode_cstr(self->nodeManager, definition_text);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_client_getLastErrorLine(apx_client_t *self)
{
   if (self != 0)
   {
      return apx_nodeManager_getLastErrorLine(self->nodeManager);
   }
   return -1;
}

apx_nodeInstance_t *apx_client_getLastAttachedNode(apx_client_t *self)
{
   if (self != 0)
   {
      return apx_nodeManager_getLastAttached(self->nodeManager);
   }
   return (apx_nodeInstance_t*) 0;
}

struct apx_fileManager_tag *apx_client_getFileManager(apx_client_t *self)
{
   if (self != 0 && self->connection != 0)
   {
      return &self->connection->base.fileManager;
   }
   return (apx_fileManager_t*) 0;
}

struct apx_nodeManager_tag *apx_client_getNodeManager(apx_client_t *self)
{
   if (self != 0 && self->connection != 0)
   {
      return &self->connection->base.nodeManager;
   }
   return (apx_nodeManager_t*) 0;
}

/*** Port Handle API ***/
void *apx_client_getPortHandle(apx_client_t *self, const char *nodeName, const char *portName)
{
   if ( (self != 0) && (portName != 0))
   {
      apx_nodeInstance_t *nodeInstance = 0;
      if (nodeName == 0)
      {
         nodeInstance = apx_client_getLastAttachedNode(self);
      }
      else
      {
         nodeInstance = apx_nodeManager_find(self->nodeManager, nodeName);
      }
      if (nodeInstance != 0)
      {
         apx_uniquePortId_t uniquePortId;
         apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
         assert(nodeInfo != 0);
         uniquePortId = apx_nodeInfo_findPortIdByName(nodeInfo, portName);
         if (uniquePortId != APX_INVALID_PORT_ID)
         {
            apx_portId_t portId;
            if ((uniquePortId & APX_PORT_ID_PROVIDE_PORT) != 0)
            {
               portId = uniquePortId & APX_PORT_ID_MASK;
               return (void*) apx_nodeInstance_getProvidePortRef(nodeInstance, portId);
            }
            else
            {
               portId = uniquePortId;
               return (void*) apx_nodeInstance_getRequirePortRef(nodeInstance, portId);
            }
         }
      }
   }
   return (void*) 0;
}

void *apx_client_getProvidePortHandleById(apx_client_t *self, const char *nodeName, apx_portId_t providePortId)
{
   if ( (self != 0) && (providePortId >= 0))
   {
      apx_nodeInstance_t *nodeInstance = 0;
      if (nodeName == 0)
      {
         nodeInstance = apx_client_getLastAttachedNode(self);
      }
      else
      {
         nodeInstance = apx_nodeManager_find(self->nodeManager, nodeName);
      }
      if (nodeInstance != 0)
      {
         return apx_nodeInstance_getProvidePortHandle(nodeInstance, providePortId);
      }
   }
   return (void*) 0;
}

void *apx_client_getRequirePortHandleById(apx_client_t *self, const char *nodeName, apx_portId_t requirePortId)
{
   if ( (self != 0) && (requirePortId >= 0))
   {
      apx_nodeInstance_t *nodeInstance = 0;
      if (nodeName == 0)
      {
         nodeInstance = apx_client_getLastAttachedNode(self);
      }
      else
      {
         nodeInstance = apx_nodeManager_find(self->nodeManager, nodeName);
      }
      if (nodeInstance != 0)
      {
         return apx_nodeInstance_getRequirePortHandle(nodeInstance, requirePortId);
      }
   }
   return (void*) 0;
}

/*** Port Data Write API ***/

apx_error_t apx_client_writePortData(apx_client_t *self, void *portHandle, const dtl_dv_t *value)
{
   if ( (self != 0) && (portHandle != 0) && (value != 0) )
   {
      uint8_t stackBuffer[MAX_STACK_BUFFER_SIZE];
      apx_error_t result;
      uint8_t *writeBuffer;
      bool isHeapAllocated = false;
      const apx_portDataProps_t *portDataProps;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      const adt_bytes_t *portProgram;
      if (!apx_portRef_isProvidePort(portRef))
      {
         return APX_INVALID_PORT_HANDLE_ERROR;
      }
      portDataProps = portRef->portDataProps;
      if (!apx_portDataProps_isPlainOldData(portDataProps))
      {
         printf("Not plain old data\n");
         return APX_NOT_IMPLEMENTED_ERROR; ///TODO: Implement dynamic array and queued signals later
      }
      if (portDataProps->dataSize > MAX_STACK_BUFFER_SIZE)
      {
         writeBuffer = (uint8_t*) malloc(portDataProps->dataSize);
         if (writeBuffer == 0)
         {
            return APX_MEM_ERROR;
         }
         isHeapAllocated = true;
      }
      else
      {
         writeBuffer = &stackBuffer[0];
      }
      SPINLOCK_ENTER(self->lock);
      if (self->vm == 0)
      {
         self->vm = apx_vm_new();
         if (self->vm == 0)
         {
            SPINLOCK_LEAVE(self->lock);
            if (isHeapAllocated) free(writeBuffer);
            return APX_MEM_ERROR;
         }
      }
      assert(self->vm != 0);
      portProgram = apx_nodeInstance_getProvidePortPackProgram(portRef->nodeInstance, apx_portRef_getPortId(portRef));
      if (portProgram == 0)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(writeBuffer);
         return APX_INVALID_PROGRAM_ERROR;
      }
      result = apx_vm_selectProgram(self->vm, portProgram);
      if (result != APX_NO_ERROR)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(writeBuffer);
         return result;
      }
      result = apx_vm_setWriteBuffer(self->vm, writeBuffer, portDataProps->dataSize);
      if (result != APX_NO_ERROR)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(writeBuffer);
         return result;
      }
      result = apx_vm_packValue(self->vm, value);
      if (result != APX_NO_ERROR)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(writeBuffer);
         return result;
      }
      SPINLOCK_LEAVE(self->lock);
      result = apx_nodeInstance_writeProvidePortData(portRef->nodeInstance, writeBuffer, portDataProps->offset, portDataProps->dataSize);
      if (isHeapAllocated) free(writeBuffer);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_writePortData_u8(apx_client_t *self, void *portHandle, uint8_t value)
{
   if (self != 0 && portHandle != 0)
   {
      apx_error_t rc;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      rc = apx_client_verifySingleInstructionProgramFromPortRef(portRef, APX_OPCODE_PACK, APX_VARIANT_U8);
      if (rc == APX_NO_ERROR)
      {
         apx_error_t result;
         SPINLOCK_ENTER(self->lock);
         result = apx_nodeInstance_writeProvidePortData(portRef->nodeInstance, &value, portRef->portDataProps->offset, UINT8_SIZE);
         SPINLOCK_LEAVE(self->lock);
         return result;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_writePortData_u16(apx_client_t *self, void *portHandle, uint16_t value)
{
   if (self != 0 && portHandle != 0)
   {
      apx_error_t rc;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      rc = apx_client_verifySingleInstructionProgramFromPortRef(portRef, APX_OPCODE_PACK, APX_VARIANT_U16);
      if (rc == APX_NO_ERROR)
      {
         apx_error_t result;
         uint8_t packedData[UINT16_SIZE];
         packLE(&packedData[0], value, UINT16_SIZE);
         SPINLOCK_ENTER(self->lock);
         result = apx_nodeInstance_writeProvidePortData(portRef->nodeInstance, &packedData[0], portRef->portDataProps->offset, UINT16_SIZE);
         SPINLOCK_LEAVE(self->lock);
         return result;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_writePortData_u32(apx_client_t *self, void *portHandle, uint32_t value)
{
   if (self != 0 && portHandle != 0)
   {
      apx_error_t rc;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      rc = apx_client_verifySingleInstructionProgramFromPortRef(portRef, APX_OPCODE_PACK, APX_VARIANT_U32);
      if (rc == APX_NO_ERROR)
      {
         apx_error_t result;
         uint8_t packedData[UINT32_SIZE];
         packLE(&packedData[0], value, UINT32_SIZE);
         SPINLOCK_ENTER(self->lock);
         result = apx_nodeInstance_writeProvidePortData(portRef->nodeInstance, &packedData[0], portRef->portDataProps->offset, UINT32_SIZE);
         SPINLOCK_LEAVE(self->lock);
         return result;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/*** Port Data Read API ***/

apx_error_t apx_client_readPortData(apx_client_t *self, void *portHandle, dtl_dv_t **dv)
{
   if ( (self != 0) && (portHandle != 0) && (dv != 0) )
   {
      uint8_t stackBuffer[MAX_STACK_BUFFER_SIZE];
      apx_error_t result;
      uint8_t *readBuffer;
      bool isHeapAllocated = false;
      const apx_portDataProps_t *portDataProps;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      const adt_bytes_t *portProgram;
      if (apx_portRef_isProvidePort(portRef))
      {
         return APX_INVALID_PORT_HANDLE_ERROR;
      }
      portDataProps = portRef->portDataProps;
      if (!apx_portDataProps_isPlainOldData(portDataProps))
      {
         printf("Not plain old data\n");
         return APX_NOT_IMPLEMENTED_ERROR; ///TODO: Implement dynamic array and queued signals later
      }
      if (portDataProps->dataSize > MAX_STACK_BUFFER_SIZE)
      {
         readBuffer = (uint8_t*) malloc(portDataProps->dataSize);
         if (readBuffer == 0)
         {
            return APX_MEM_ERROR;
         }
         isHeapAllocated = true;
      }
      else
      {
         readBuffer = &stackBuffer[0];
      }
      assert(readBuffer != 0);
      result = apx_nodeInstance_readRequirePortData(portRef->nodeInstance, readBuffer, portDataProps->offset, portDataProps->dataSize);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      SPINLOCK_ENTER(self->lock);
      if (self->vm == 0)
      {
         self->vm = apx_vm_new();
         if (self->vm == 0)
         {
            SPINLOCK_LEAVE(self->lock);
            if (isHeapAllocated) free(readBuffer);
            return APX_MEM_ERROR;
         }
      }
      assert(self->vm != 0);
      portProgram = apx_nodeInstance_getRequirePortUnpackProgram(portRef->nodeInstance, apx_portRef_getPortId(portRef));
      if (portProgram == 0)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(readBuffer);
         return APX_INVALID_PROGRAM_ERROR;
      }
      result = apx_vm_selectProgram(self->vm, portProgram);
      if (result != APX_NO_ERROR)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(readBuffer);
         return result;
      }
      result = apx_vm_setReadBuffer(self->vm, readBuffer, portDataProps->dataSize);
      if (result != APX_NO_ERROR)
      {
         SPINLOCK_LEAVE(self->lock);
         if (isHeapAllocated) free(readBuffer);
         return result;
      }
      result = apx_vm_unpackValue(self->vm, dv);
      SPINLOCK_LEAVE(self->lock);
      if (isHeapAllocated) free(readBuffer);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_readPortData_u8(apx_client_t *self, void *portHandle, uint8_t *value)
{
   if ( (self != 0) && (portHandle != 0)  && (value != 0) )
   {
      apx_error_t rc;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      rc = apx_client_verifySingleInstructionProgramFromPortRef(portRef, APX_OPCODE_UNPACK, APX_VARIANT_U8);
      if (rc == APX_NO_ERROR)
      {
         SPINLOCK_ENTER(self->lock);
         rc = apx_nodeInstance_readRequirePortData(portRef->nodeInstance, value, portRef->portDataProps->offset, UINT8_SIZE);
         SPINLOCK_LEAVE(self->lock);
         return rc;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_readPortData_u16(apx_client_t *self, void *portHandle, uint16_t *value)
{
   if ( (self != 0) && (portHandle != 0)  && (value != 0) )
   {
      apx_error_t rc;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      rc = apx_client_verifySingleInstructionProgramFromPortRef(portRef, APX_OPCODE_UNPACK, APX_VARIANT_U16);
      if (rc == APX_NO_ERROR)
      {
         uint8_t packedData[UINT16_SIZE];
         SPINLOCK_ENTER(self->lock);
         rc = apx_nodeInstance_readRequirePortData(portRef->nodeInstance, &packedData[0], portRef->portDataProps->offset, UINT16_SIZE);
         SPINLOCK_LEAVE(self->lock);
         if (rc == APX_NO_ERROR)
         {
            *value = (uint16_t) unpackLE(&packedData[0], UINT16_SIZE);
         }
         return rc;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_readPortData_u32(apx_client_t *self, void *portHandle, uint32_t *value)
{
   if ( (self != 0) && (portHandle != 0)  && (value != 0) )
   {
      apx_error_t rc;
      apx_portRef_t *portRef = (apx_portRef_t*) portHandle;
      rc = apx_client_verifySingleInstructionProgramFromPortRef(portRef, APX_OPCODE_UNPACK, APX_VARIANT_U32);
      if (rc == APX_NO_ERROR)
      {
         uint8_t packedData[UINT32_SIZE];
         SPINLOCK_ENTER(self->lock);
         rc = apx_nodeInstance_readRequirePortData(portRef->nodeInstance, &packedData[0], portRef->portDataProps->offset, UINT32_SIZE);
         SPINLOCK_LEAVE(self->lock);
         if (rc == APX_NO_ERROR)
         {
            *value = unpackLE(&packedData[0], UINT32_SIZE);
         }
         return rc;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


/////////////////////// BEGIN CLIENT INTERNAL API /////////////////////
void apx_clientInternal_onConnect(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      apx_client_triggerConnectedEventOnListeners(self, connection);
   }
}

void apx_clientInternal_onDisconnect(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      apx_client_triggerDisconnectedEventOnListeners(self, connection);
   }
}

void apx_clientInternal_requirePortDataWriteNotify(apx_client_t *self, apx_clientConnectionBase_t *connection, apx_nodeInstance_t *nodeInstance, uint32_t offset, const uint8_t *data, uint32_t len)
{
   (void) connection;
   if ( (self != 0) && (nodeInstance != 0) )
   {
      apx_portId_t requirePortId = 0;
      void *portHandle = 0;
      uint32_t endOffset = offset + len;
      apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
      assert(nodeInfo != 0);
      while(offset < endOffset)
      {
         apx_portDataProps_t *portDataProps;
         requirePortId = apx_nodeInfo_findRequirePortIdFromByteOffset(nodeInfo, offset);
         if (requirePortId < 0)
         {
            printf("[APX-CLIENT] Write at invalid offset %d\n", (int) offset);
            return;
         }
         portDataProps = apx_nodeInfo_getRequirePortDataProps(nodeInfo, requirePortId);
         assert(portDataProps != 0);
         portHandle = (void*) apx_nodeInstance_getRequirePortRef(nodeInstance, requirePortId);
         assert(portHandle != 0);
         apx_client_triggerRequirePortDataWriteEventOnListeners(self, nodeInstance, requirePortId, portHandle);
         offset += portDataProps->dataSize;
      }
   }
}

/////////////////////// END CLIENT INTERNAL API /////////////////////

/////////////////////// BEGIN UNIT TEST API /////////////////////
#ifdef UNIT_TEST

#define APX_CLIENT_RUN_CYCLES 10

void apx_client_run(apx_client_t *self)
{
   if (self!=0 && (self->connection != 0))
   {
      int32_t i;
      for(i=0;i<APX_CLIENT_RUN_CYCLES;i++)
      {
         apx_clientConnectionBase_run(self->connection);
      }
   }
}
#endif

/////////////////////// END UNIT TEST API /////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_client_triggerConnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   SPINLOCK_ENTER(self->eventListenerLock);
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientConnect1 != 0))
      {
         listener->clientConnect1(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
   SPINLOCK_LEAVE(self->eventListenerLock);
}

static void apx_client_triggerDisconnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   SPINLOCK_ENTER(self->eventListenerLock);
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientDisconnect1 != 0))
      {
         listener->clientDisconnect1(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
   SPINLOCK_LEAVE(self->eventListenerLock);
}

static void apx_client_triggerRequirePortDataWriteEventOnListeners(apx_client_t *self, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle)
{
   SPINLOCK_ENTER(self->eventListenerLock);
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->requirePortWrite1 != 0))
      {
         listener->requirePortWrite1(listener->arg, nodeInstance, requirePortId, portHandle);
      }
      iter = adt_list_iter_next(iter);
   }
   SPINLOCK_LEAVE(self->eventListenerLock);
}

static void apx_client_attachLocalNodesToConnection(apx_client_t *self)
{
   if (self->connection != 0)
   {
      adt_ary_t *nodeList = adt_ary_new( (void(*)(void*)) 0);
      if (nodeList != 0)
      {
         int32_t i;
         int32_t numNodes;
         numNodes = apx_nodeManager_values(self->nodeManager, nodeList);
         for (i=0; i<numNodes; i++)
         {
            apx_nodeInstance_t *nodeInstance = (apx_nodeInstance_t*) adt_ary_value(nodeList, i);
            apx_clientConnectionBase_attachNodeInstance(self->connection, nodeInstance);
         }
         adt_ary_delete(nodeList);
      }
   }
}

static apx_error_t apx_client_verifySingleInstructionProgramFromPortRef(apx_portRef_t *portRef, uint8_t opcode, uint8_t variant)
{
   apx_nodeInfo_t *nodeInfo;
   const adt_bytes_t *program = 0;
   nodeInfo = apx_nodeInstance_getNodeInfo(portRef->nodeInstance);
   assert(nodeInfo != 0);

   if (apx_portRef_isProvidePort(portRef))
   {
      program = apx_nodeInfo_getProvidePortPackProgram(nodeInfo, apx_portRef_getPortId(portRef));
   }
   else
   {
      program = apx_nodeInfo_getRequirePortUnpackProgram(nodeInfo, apx_portRef_getPortId(portRef));
   }

   if (program != 0)
   {
      return apx_client_verifySingleInstructionProgram(program, opcode, variant);
   }
   else
   {
      return APX_INVALID_PROGRAM_ERROR;
   }
}

static apx_error_t apx_client_verifySingleInstructionProgram(const adt_bytes_t *program, uint8_t expectedOpcode, uint8_t expectedVariant)
{
   if (program != 0)
   {
      const uint8_t *code;
      int32_t length;
      code = adt_bytes_constData(program);
      length = adt_bytes_length(program);
      if (length == APX_VM_HEADER_SIZE + APX_VM_INSTRUCTION_SIZE)
      {
         uint8_t opcode = 0u;
         uint8_t variant = 0u;
         uint8_t flags = 0u;
         apx_error_t rc = apx_vm_decodeInstruction(code[APX_VM_HEADER_SIZE], &opcode, &variant, &flags);
         if ( (rc == APX_NO_ERROR) )
         {
            if ( (flags == 0) && (opcode == expectedOpcode) && (variant == expectedVariant))
            {
               return APX_NO_ERROR;
            }
            else
            {
               return APX_INVALID_INSTRUCTION_ERROR;
            }
         }
         else
         {
            return rc;
         }
      }
      else
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


