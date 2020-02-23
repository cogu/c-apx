/*****************************************************************************
* \file      apx_nodeData.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Container for data in an APX node that changes over time (port values, port connection count etc.)
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
#ifndef APX_NODE_DATA_H
#define APX_NODE_DATA_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#ifndef APX_EMBEDDED
#  ifndef _WIN32
     //Linux-based system
#    include <pthread.h>
#  else
     //Windows-based system
#    include <Windows.h>
#  endif
#  include "osmacro.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeInstance_tag;
struct apx_portDataProps_tag;

typedef struct apx_nodeDataBuffers_tag
{
   uint8_t *definitionDataBuf;
   uint8_t *requirePortPortDataBuf;
   uint8_t *providePortDataBuf;
   apx_size_t definitionDataLen;
   apx_size_t requirePortDataLen;
   apx_size_t providePortDataLen;
   apx_connectionCount_t *requirePortConnectionCount;
   apx_connectionCount_t *providePortConnectionCount;
   apx_portCount_t numRequirePorts;
   apx_portCount_t numProvidePorts;
   uint8_t definitionChecksumType;
   uint8_t definitionChecksumData[APX_CHECKSUMLEN_SHA256];
} apx_nodeDataBuffers_t;

typedef struct apx_nodeData_tag
{
   bool isWeakref; //when true all pointers in this object is owned by some other part of the program. if false then all pointers are created/freed by this class.
   uint8_t definitionChecksumType;
   uint8_t *definitionDataBuf;
   uint8_t *requirePortDataBuf;
   uint8_t *providePortDataBuf;
   apx_size_t requirePortDataLen;
   apx_size_t providePortDataLen;
   apx_size_t definitionDataLen;
   uint8_t definitionChecksumData[APX_CHECKSUMLEN_SHA256];
   apx_connectionCount_t *requirePortConnectionCount; //Number of active connections to each require-port
   apx_connectionCount_t *providePortConnectionCount; //Number of active connections to each provide-port
   uint32_t portConnectionsTotal; //Total number of active port connections
   apx_portCount_t numRequirePorts; //Number of require-ports in this node
   apx_portCount_t numProvidePorts; //Number of provide-ports in this node
#ifndef APX_EMBEDDED
   SPINLOCK_T requirePortDataLock;
   SPINLOCK_T providePortDataLock;
   SPINLOCK_T definitionDataLock;
   SPINLOCK_T internalLock;
#endif
   struct apx_nodeInstance_tag *parent; //pointer to parent nodeInstance (weak reference)
} apx_nodeData_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

////////////////// Constructor/Destructor //////////////////
void apx_nodeData_create(apx_nodeData_t *self, apx_nodeDataBuffers_t *buffers);
void apx_nodeData_destroy(apx_nodeData_t *self);
#ifndef APX_EMBEDDED
apx_nodeData_t *apx_nodeData_new(void);
void apx_nodeData_delete(apx_nodeData_t *self);
void apx_nodeData_vdelete(void *arg);
#endif

////////////////// Data Buffer API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_nodeData_createDefinitionBuffer(apx_nodeData_t *self, apx_size_t bufferLen);
#endif
void apx_nodeData_lockDefinitionData(apx_nodeData_t *self);
void apx_nodeData_unlockDefinitionData(apx_nodeData_t *self);
const uint8_t *apx_nodeData_getDefinitionDataBuf(apx_nodeData_t *self);
apx_size_t apx_nodeData_getDefinitionDataLen(apx_nodeData_t *self);
uint8_t apx_nodeData_getDefinitionChecksumType(apx_nodeData_t *self);
const uint8_t* apx_nodeData_getDefinitionChecksumData(apx_nodeData_t *self);
apx_error_t apx_nodeData_writeDefinitionData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readDefinitionData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_setDefinitionChecksumData(apx_nodeData_t *self, uint8_t checksumType, uint8_t *checksumData);

#ifndef APX_EMBEDDED
apx_error_t apx_nodeData_createRequirePortBuffer(apx_nodeData_t *self, apx_size_t bufferLen);
#endif
apx_size_t apx_nodeData_getRequirePortDataLen(apx_nodeData_t *self);
apx_error_t apx_nodeData_writeRequirePortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);
apx_error_t apx_nodeData_readRequirePortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, apx_size_t len);


#ifndef APX_EMBEDDED
apx_error_t apx_nodeData_createProvidePortBuffer(apx_nodeData_t *self, apx_size_t bufferLen);
#endif
apx_size_t apx_nodeData_getProvidePortDataLen(apx_nodeData_t *self);
apx_error_t apx_nodeData_writeProvidePortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);
apx_error_t apx_nodeData_readProvidePortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, apx_size_t len);

apx_error_t apx_nodeData_updatePortDataDirect(apx_nodeData_t *destNodeData, const struct apx_portDataProps_tag *destDatProps,
      apx_nodeData_t *srcNodeData, const struct apx_portDataProps_tag *srcDataProps);

////////////////// NodeInstance (parent) API //////////////////
void apx_nodeData_setNodeInstance(apx_nodeData_t *self, struct apx_nodeInstance_tag *node);
struct apx_nodeInstance_tag *apx_nodeData_getNodeInstance(apx_nodeData_t *self);

////////////////// Port Connection Count API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_nodeData_createRequirePortConnectionCountBuffer(apx_nodeData_t *self, apx_portCount_t numRequirePorts);
apx_error_t apx_nodeData_createProvidePortConnectionCountBuffer(apx_nodeData_t *self, apx_portCount_t numProvidePorts);
#endif
apx_connectionCount_t apx_nodeData_getRequirePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
apx_connectionCount_t apx_nodeData_getProvidePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_incRequirePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_incProvidePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_decRequirePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_decProvidePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
uint32_t apx_nodeData_getPortConnectionsTotal(apx_nodeData_t *self);

////////////////// Utility Functions //////////////////
const char *apx_nodeData_getName(apx_nodeData_t *self);
bool apx_nodeData_isComplete(apx_nodeData_t *self);
uint32_t apx_nodeData_getConnectionId(apx_nodeData_t *self);

#endif //APX_NODE_DATA2_H
