/*****************************************************************************
* \file      apx_nodeData2.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Container for data in an APX node that changes over time (port values, port connection count etc.)
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
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "apx_nodeData2.h"
#include "apx_nodeInstance.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
////////////////// Constructor/Destructor //////////////////
void apx_nodeData2_create(apx_nodeData2_t *self, apx_nodeData2Buffers_t *buffers)
{
   if (self != 0)
   {
      if (buffers != 0)
      {
         self->isWeakref = true;
         self->definitionDataBuf = buffers->definitionDataBuf;
         self->requirePortDataBuf = buffers->requirePortPortDataBuf;
         self->providePortDataBuf = buffers->providePortDataBuf;
         self->definitionDataLen = buffers->definitionDataLen;
         self->requirePortDataLen = buffers->requirePortDataLen;
         self->providePortDataLen = buffers->providePortDataLen;
         self->requirePortConnectionCount = buffers->requirePortConnectionCount;
         self->providePortConnectionCount = buffers->providePortConnectionCount;
         self->numRequirePorts = buffers->numRequirePorts;
         self->numProvidePorts = buffers->numProvidePorts;
         self->definitionChecksumType = buffers->definitionChecksumType;
         memcpy(&self->definitionChecksumData[0], &buffers->definitionChecksumData[0], APX_CHECKSUMLEN_SHA256);
      }
      else
      {
         self->isWeakref = false;
         self->definitionDataBuf = (uint8_t*) 0;
         self->requirePortDataBuf = (uint8_t*) 0;
         self->providePortDataBuf = (uint8_t*) 0;
         self->definitionDataLen = 0u;
         self->requirePortDataLen = 0u;
         self->providePortDataLen = 0u;
         self->requirePortConnectionCount = 0u;
         self->providePortConnectionCount = 0u;
         self->numRequirePorts = 0u;
         self->numProvidePorts = 0u;
         self->definitionChecksumType = APX_CHECKSUM_NONE;
         memset(&self->definitionChecksumData[0], 0, APX_CHECKSUMLEN_SHA256);
      }
      self->portConnectionsTotal  = 0u;
      self->parent = (apx_nodeInstance_t*) 0;
#ifndef APX_EMBEDDED
      SPINLOCK_INIT(self->requirePortDataLock);
      SPINLOCK_INIT(self->providePortDataLock);
      SPINLOCK_INIT(self->definitionDataLock);
      SPINLOCK_INIT(self->internalLock);
#endif
   }
}

void apx_nodeData2_destroy(apx_nodeData2_t *self)
{
   if ( (self != 0)  )
   {
#ifndef APX_EMBEDDED
      if (!self->isWeakref)
      {
         if (self->definitionDataBuf != 0)
         {
            free(self->definitionDataBuf);
         }
         if (self->requirePortDataBuf != 0)
         {
            free(self->requirePortDataBuf);
         }
         if (self->providePortDataBuf != 0)
         {
            free(self->providePortDataBuf);
         }
         if (self->requirePortConnectionCount != 0)
         {
            free(self->requirePortConnectionCount);
         }
         if (self->providePortConnectionCount != 0)
         {
            free(self->providePortConnectionCount);
         }
      }
      SPINLOCK_DESTROY(self->requirePortDataLock);
      SPINLOCK_DESTROY(self->providePortDataLock);
      SPINLOCK_DESTROY(self->definitionDataLock);
      SPINLOCK_DESTROY(self->internalLock);
#endif
   }
}
#ifndef APX_EMBEDDED
apx_nodeData2_t *apx_nodeData2_new(void)
{
   apx_nodeData2_t *self = (apx_nodeData2_t*) malloc(sizeof(apx_nodeData2_t));
   if (self != 0)
   {
      apx_nodeData2_create(self, (apx_nodeData2Buffers_t*) 0);
   }
   return self;
}

void apx_nodeData2_delete(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      apx_nodeData2_destroy(self);
      free(self);
   }
}
void apx_nodeData2_vdelete(void *arg)
{
   apx_nodeData2_delete((apx_nodeData2_t*) arg);
}
#endif

////////////////// Data Buffer API //////////////////

#ifndef APX_EMBEDDED
apx_error_t apx_nodeData2_createDefinitionBuffer(apx_nodeData2_t *self, apx_size_t bufferLen)
{
   if (self != 0)
   {
      uint8_t *definitionDataBuf = (uint8_t*) malloc(bufferLen);
      if (definitionDataBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      self->definitionDataBuf = definitionDataBuf;
      self->definitionDataLen = bufferLen;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#endif

void apx_nodeData2_lockDefinitionData(apx_nodeData2_t *self)
{
   if (self != 0)
   {
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->definitionDataLock);
#endif
   }
}

void apx_nodeData2_unlockDefinitionData(apx_nodeData2_t *self)
{
   if (self != 0)
   {
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->definitionDataLock);
#endif
   }
}

const uint8_t *apx_nodeData2_getDefinitionDataBuf(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return self->definitionDataBuf;
   }
   return (const uint8_t*) 0;
}


apx_size_t apx_nodeData2_getDefinitionDataLen(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return self->definitionDataLen;
   }
   return (apx_size_t) 0u;
}

uint8_t apx_nodeData2_getDefinitionChecksumType(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return self->definitionChecksumType;
   }
   return (uint8_t) APX_CHECKSUM_NONE;
}

const uint8_t* apx_nodeData2_getDefinitionChecksumData(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return (const uint8_t*) &self->definitionChecksumData;
   }
   return (const uint8_t*) 0;
}

apx_error_t apx_nodeData2_writeDefinitionData(apx_nodeData2_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
      apx_nodeData2_lockDefinitionData(self);
      if ( (offset+len) > self->definitionDataLen)
      {
         retval = APX_INVALID_ARGUMENT_ERROR;
      }
      else
      {
         memcpy(&self->definitionDataBuf[offset], src, len);
      }
      apx_nodeData2_unlockDefinitionData(self);
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

apx_error_t apx_nodeData2_readDefinitionData(apx_nodeData2_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   apx_error_t retval = APX_NO_ERROR;
   if( (self != 0) && (dest != 0) )
   {
      apx_nodeData2_lockDefinitionData(self);
      if ( (offset+len) > self->definitionDataLen)
      {
         retval = APX_INVALID_ARGUMENT_ERROR;
      }
      else
      {
         memcpy(dest, &self->definitionDataBuf[offset], len);
      }
      apx_nodeData2_unlockDefinitionData(self);
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

apx_error_t apx_nodeData2_setDefinitionChecksumData(apx_nodeData2_t *self, uint8_t checksumType, uint8_t *checksumData)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != 0) && (checksumType <= APX_CHECKSUM_SHA256) && (checksumData != 0) )
   {
      apx_nodeData2_lockDefinitionData(self);
      if (checksumType == APX_CHECKSUM_SHA256)
      {
         self->definitionChecksumType = checksumType;
         memcpy(&self->definitionChecksumData[0], checksumData, APX_CHECKSUMLEN_SHA256);
      }
      else
      {
         retval = APX_INVALID_ARGUMENT_ERROR;
      }
      apx_nodeData2_unlockDefinitionData(self);   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

#ifndef APX_EMBEDDED
apx_error_t apx_nodeData2_createRequirePortBuffer(apx_nodeData2_t *self, apx_size_t bufferLen)
{
   if (self != 0)
   {
      uint8_t *requirePortDataBuf = (uint8_t*) malloc(bufferLen);
      if (requirePortDataBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      self->requirePortDataBuf = requirePortDataBuf;
      self->requirePortDataLen = bufferLen;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#endif

apx_size_t apx_nodeData2_getRequirePortDataLen(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return self->requirePortDataLen;
   }
   return (apx_size_t) 0u;
}

apx_error_t apx_nodeData2_writeRequirePortData(apx_nodeData2_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->requirePortDataLock);
#endif
   if ( (offset+len) > self->requirePortDataLen)
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   else
   {
      memcpy(&self->requirePortDataBuf[offset], src, len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->requirePortDataLock);
#endif
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

apx_error_t apx_nodeData2_readRequirePortData(apx_nodeData2_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   apx_error_t retval = APX_NO_ERROR;
   if( (self != 0) && (dest != 0) )
   {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->requirePortDataLock);
#endif
      if ( (offset+len) > self->requirePortDataLen)
      {
         retval = APX_INVALID_ARGUMENT_ERROR;
      }
      else
      {
         memcpy(dest, &self->requirePortDataBuf[offset], len);
      }
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->requirePortDataLock);
#endif
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

#ifndef APX_EMBEDDED
apx_error_t apx_nodeData2_createProvidePortBuffer(apx_nodeData2_t *self, apx_size_t bufferLen)
{
   if (self != 0)
   {
      uint8_t *providePortDataBuf = (uint8_t*) malloc(bufferLen);
      if (providePortDataBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      self->providePortDataBuf = providePortDataBuf;
      self->providePortDataLen = bufferLen;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#endif

apx_size_t apx_nodeData2_getProvidePortDataLen(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return self->providePortDataLen;
   }
   return (apx_size_t) 0u;
}

apx_error_t apx_nodeData2_writeProvidePortData(apx_nodeData2_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->providePortDataLock);
#endif
   if ( (offset+len) > self->providePortDataLen)
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   else
   {
      memcpy(&self->providePortDataBuf[offset], src, len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->providePortDataLock);
#endif
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

apx_error_t apx_nodeData2_readProvidePortData(apx_nodeData2_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   apx_error_t retval = APX_NO_ERROR;
   if( (self != 0) && (dest != 0) )
   {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->providePortDataLock);
#endif
      if ( (offset+len) > self->providePortDataLen)
      {
         retval = APX_INVALID_ARGUMENT_ERROR;
      }
      else
      {
         memcpy(dest, &self->providePortDataBuf[offset], len);
      }
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->providePortDataLock);
#endif
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

////////////////// NodeInstance (parent) API //////////////////

void apx_nodeData2_setNodeInstance(apx_nodeData2_t *self, struct apx_nodeInstance_tag *node)
{
   if (self != 0)
   {
      self->parent = node;
   }
}

struct apx_nodeInstance_tag *apx_nodeData2_getNodeInstance(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      return self->parent;
   }
   return (struct apx_nodeInstance_tag*) 0;
}

////////////////// Port Connection Count API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_nodeData2_createRequirePortConnectionCountBuffer(apx_nodeData2_t *self, apx_portCount_t numRequirePorts)
{
   if (self != 0)
   {
      apx_connectionCount_t *connectionCountBuf = (apx_connectionCount_t*) malloc(numRequirePorts*sizeof(apx_connectionCount_t));
      if (connectionCountBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      self->requirePortConnectionCount = connectionCountBuf;
      self->numRequirePorts = numRequirePorts;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData2_createProvidePortConnectionCountBuffer(apx_nodeData2_t *self, apx_portCount_t numProvidePorts)
{
   if (self != 0)
   {
      apx_connectionCount_t *connectionCountBuf = (apx_connectionCount_t*) malloc(numProvidePorts*sizeof(apx_connectionCount_t));
      if (connectionCountBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      self->providePortConnectionCount = connectionCountBuf;
      self->numProvidePorts = numProvidePorts;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#endif //APX_EMBEDDED

apx_connectionCount_t apx_nodeData2_getRequirePortConnectionCount(apx_nodeData2_t *self, apx_portId_t portId)
{
   apx_connectionCount_t retval = 0;
   if ( (self != 0) && (self->requirePortConnectionCount != 0) && (portId < self->numRequirePorts) )
   {
      retval = self->requirePortConnectionCount[portId];
   }
   return retval;
}


apx_connectionCount_t apx_nodeData2_getProvidePortConnectionCount(apx_nodeData2_t *self, apx_portId_t portId)
{
   apx_connectionCount_t retval = 0;
   if ( (self != 0) && (self->providePortConnectionCount != 0) && (portId < self->numProvidePorts) )
   {
      retval = self->providePortConnectionCount[portId];
   }
   return retval;
}

void apx_nodeData2_incRequirePortConnectionCount(apx_nodeData2_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->requirePortConnectionCount != 0) && (portId < self->numRequirePorts) )
   {
      if (self->requirePortConnectionCount[portId] < APX_CONNECTION_COUNT_MAX)
      {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->internalLock);
#endif
         self->requirePortConnectionCount[portId]++;
         self->portConnectionsTotal++;
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->internalLock);
#endif
      }
   }
}

void apx_nodeData2_incProvidePortConnectionCount(apx_nodeData2_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->providePortConnectionCount != 0) && (portId < self->numProvidePorts) )
   {
      if (self->providePortConnectionCount[portId] < APX_CONNECTION_COUNT_MAX)
      {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->internalLock);
#endif
         self->providePortConnectionCount[portId]++;
         self->portConnectionsTotal++;
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->internalLock);
#endif
      }
   }
}

void apx_nodeData2_decRequirePortConnectionCount(apx_nodeData2_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->requirePortConnectionCount != 0) && (portId < self->numRequirePorts) )
   {
      if (self->requirePortConnectionCount[portId] > 0u)
      {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->internalLock);
#endif
         self->requirePortConnectionCount[portId]--;
         self->portConnectionsTotal--;
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->internalLock);
#endif
      }
   }
}

void apx_nodeData2_decProvidePortConnectionCount(apx_nodeData2_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->providePortConnectionCount != 0) && (portId < self->numProvidePorts) )
   {
      if (self->providePortConnectionCount[portId] > 0u)
      {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->internalLock);
#endif
         self->providePortConnectionCount[portId]--;
         self->portConnectionsTotal--;
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->internalLock);
#endif
      }
   }
}

uint32_t apx_nodeData2_getPortConnectionsTotal(apx_nodeData2_t *self)
{
   if (self != 0)
   {
      uint32_t retval;
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->internalLock);
#endif
      retval = self->portConnectionsTotal;
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->internalLock);
#endif
      return retval;
   }
   return 0;
}

////////////////// Utility Functions //////////////////
const char *apx_nodeData2_getName(apx_nodeData2_t *self)
{
   return "Unkown";
}

bool apx_nodeData2_isComplete(apx_nodeData2_t *self)
{
   return false;
}

uint32_t apx_nodeData2_getConnectionId(apx_nodeData2_t *self)
{
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


