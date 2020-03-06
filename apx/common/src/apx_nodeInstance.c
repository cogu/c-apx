/*****************************************************************************
* \file      apx_nodeInstance.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Parent container for all things node-related.
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
#include <string.h>
#include <assert.h>
#include <stdio.h> //DEBUG ONLY
#include "apx_nodeInstance.h"
#include "apx_connectionBase.h"
#include "apx_util.h"
#include "rmf.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define STACK_DATA_BUF_SIZE 256

typedef apx_portDataProps_t* (apx_getPortDataPropsFunc)(const apx_nodeInfo_t *self, apx_portId_t portId);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_nodeInstance_definitionFileWriteNotify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len);
static apx_error_t apx_nodeInstance_definitionFileOpenNotify(void *arg, struct apx_file_tag *file);
static apx_error_t apx_nodeInstance_definitionFileReadData(void *arg, apx_file_t*file, uint32_t offset, uint8_t *dest, uint32_t len);
static apx_error_t apx_nodeInstance_createFileInfo(apx_nodeInstance_t *self, const char *fileExtension, uint32_t fileSize, apx_fileInfo_t *fileInfo);
static apx_error_t apx_nodeInstance_providePortDataFileWriteNotify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len);
static apx_error_t apx_nodeInstance_providePortDataFileOpenNotify(void *arg, struct apx_file_tag *file);
static apx_error_t apx_nodeInstance_requirePortDataFileWriteNotify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len);
static apx_error_t apx_nodeInstance_requirePortDataFileOpenNotify(void *arg, struct apx_file_tag *file);
static void apx_nodeInstance_initPortRefs(apx_nodeInstance_t *self, apx_portRef_t *portRefs, apx_portCount_t numPorts, uint32_t portIdMask, apx_getPortDataPropsFunc *getPortDataProps);
static apx_error_t apx_nodeInstance_routeProvidePortDataToRequirePortByRef(apx_portRef_t *providePortRef, apx_portRef_t *requirePortRef);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_nodeInstance_create(apx_nodeInstance_t *self, apx_mode_t mode)
{
   if ( (self != 0) && ( (mode == APX_CLIENT_MODE) || (mode == APX_SERVER_MODE) ))
   {
      memset(self, 0, sizeof(apx_nodeInstance_t));
      self->mode = mode;
      self->requirePortDataState = APX_REQUIRE_PORT_DATA_STATE_INIT;
      self->providePortDataState = APX_PROVIDE_PORT_DATE_STATE_INIT;
      MUTEX_INIT(self->connectorTableLock);
   }
}

void apx_nodeInstance_destroy(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      if (self->parseTree != 0)
      {
         apx_node_delete(self->parseTree);
      }
      if (self->connectorTable != 0)
      {
         apx_portCount_t numProvidePorts;
         apx_portId_t portId;
         assert(self->nodeInfo != 0);
         numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
         assert(numProvidePorts > 0);
         MUTEX_LOCK(self->connectorTableLock);
         for(portId = 0; portId < numProvidePorts; portId++)
         {
            apx_portConnectorList_destroy(&self->connectorTable[portId]);
         }
         free(self->connectorTable);
         self->connectorTable = (apx_portConnectorList_t*) 0;
         MUTEX_UNLOCK(self->connectorTableLock);
      }
      if (self->nodeInfo != 0)
      {
         apx_nodeInfo_delete(self->nodeInfo);
      }
      if (self->nodeData != 0)
      {
         apx_nodeData_delete(self->nodeData);
      }
      if (self->requirePortReferences != 0)
      {
         free(self->requirePortReferences);
         self->requirePortReferences = 0;
      }
      if (self->providePortReferences != 0)
      {
         free(self->providePortReferences);
         self->providePortReferences = 0;
      }
      if (self->requirePortChanges != 0)
      {
         apx_portConnectorChangeTable_delete(self->requirePortChanges);
      }
      if (self->providePortChanges != 0)
      {
         apx_portConnectorChangeTable_delete(self->providePortChanges);
      }
      MUTEX_DESTROY(self->connectorTableLock);
   }
}

apx_nodeInstance_t *apx_nodeInstance_new(apx_mode_t mode)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) malloc(sizeof(apx_nodeInstance_t));
   if(self != 0)
   {
      apx_nodeInstance_create(self, mode);
      apx_nodeData_t *nodeData = apx_nodeInstance_createNodeData(self);
      if (nodeData == 0)
      {
         free(self);
         self = (apx_nodeInstance_t*) 0;
      }
   }
   return self;
}

void apx_nodeInstance_delete(apx_nodeInstance_t *self)
{
   if(self != 0)
   {
      apx_nodeInstance_destroy(self);
      free(self);
   }
}

void apx_nodeInstance_vdelete(void *arg)
{
   apx_nodeInstance_delete((apx_nodeInstance_t*) arg);
}

apx_nodeData_t* apx_nodeInstance_createNodeData(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      if (self->nodeData == 0)
      {
         self->nodeData = apx_nodeData_new();
      }
      return self->nodeData;
   }
   return (apx_nodeData_t*) 0;
}

apx_nodeData_t* apx_nodeInstance_getNodeData(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      return self->nodeData;
   }
   return (apx_nodeData_t*) 0;
}




/**
 * SERVER MODE ONLY
 */
apx_error_t apx_nodeInstance_parseDefinition(apx_nodeInstance_t *self, apx_parser_t *parser)
{
   if ( (self != 0) && (parser != 0) )
   {
      if ( self->nodeData != 0)
      {
         apx_size_t definitionLen = apx_nodeData_getDefinitionDataLen(self->nodeData);
         if (definitionLen > 0)
         {
            apx_node_t *parseTree;
            apx_nodeData_lockDefinitionData(self->nodeData);
            parseTree = apx_parser_parseBuffer(parser, apx_nodeData_getDefinitionDataBuf(self->nodeData), definitionLen);
            apx_nodeData_unlockDefinitionData(self->nodeData);
            if (parseTree != 0)
            {
               apx_parser_clearNodes(parser);
               self->parseTree = parseTree;
               return APX_NO_ERROR;
            }
            else
            {
               return apx_parser_getLastError(parser);
            }
         }
         return APX_LENGTH_ERROR;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_node_t *apx_nodeInstance_getParseTree(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      return self->parseTree;
   }
   return (apx_node_t*) 0;
}

apx_error_t apx_nodeInstance_createDefinitionBuffer(apx_nodeInstance_t *self, apx_size_t bufferLen)
{
   if ( (self != 0) && (bufferLen > 0) && (bufferLen <= APX_MAX_DEFINITION_LEN))
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData_createDefinitionBuffer(self->nodeData, bufferLen);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_createPortDataBuffers(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeInfo_t *nodeInfo;
      apx_nodeData_t *nodeData;
      apx_size_t requirePortDataLen;
      apx_size_t providePortDataLen;
      assert(self != 0);
      nodeInfo = apx_nodeInstance_getNodeInfo(self);
      nodeData = apx_nodeInstance_getNodeData(self);
      if(nodeInfo == 0 || nodeData == 0)
      {
         return APX_NULL_PTR_ERROR;
      }
      requirePortDataLen = apx_nodeInfo_calcRequirePortDataLen(nodeInfo);
      providePortDataLen = apx_nodeInfo_calcProvidePortDataLen(nodeInfo);
      if (providePortDataLen > 0u)
      {
         retval = apx_nodeData_createProvidePortBuffer(nodeData, providePortDataLen);
         if (retval == APX_NO_ERROR)
         {
            apx_size_t initDataSize = apx_nodeInfo_getProvidePortInitDataSize(nodeInfo);
            if (initDataSize == providePortDataLen)
            {
               const uint8_t *initData = apx_nodeInfo_getProvidePortInitDataPtr(nodeInfo);
               assert(initData != 0);
               apx_nodeData_writeProvidePortData(nodeData, initData, 0, initDataSize);
            }
            else
            {
               return APX_LENGTH_ERROR;
            }
         }
      }
      if ( (retval == APX_NO_ERROR) && (requirePortDataLen > 0u))
      {
         retval = apx_nodeData_createRequirePortBuffer(nodeData, requirePortDataLen);
         if (retval == APX_NO_ERROR)
         {
            apx_size_t initDataSize = apx_nodeInfo_getRequirePortInitDataSize(nodeInfo);
            if (initDataSize == requirePortDataLen)
            {
               const uint8_t *initData = apx_nodeInfo_getRequirePortInitDataPtr(nodeInfo);
               assert(initData != 0);
               apx_nodeData_writeRequirePortData(nodeData, initData, 0, initDataSize);
            }
            else
            {
               return APX_LENGTH_ERROR;
            }
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_updatePortDataDirect(apx_nodeInstance_t *destNodeInstance, const apx_portDataProps_t *destDataProps, apx_nodeInstance_t *srcNodeInstance, const apx_portDataProps_t *srcDataProps)
{
   if (destNodeInstance != 0 && srcNodeInstance != 0)
   {
      assert(destNodeInstance->nodeData != 0);
      assert(srcNodeInstance->nodeData != 0);
      return apx_nodeData_updatePortDataDirect(destNodeInstance->nodeData, destDataProps, srcNodeInstance->nodeData, srcDataProps);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * destPortId must reference a require-port in destNodeData. Likewise, srcPortId must reference a provide port in srcNodeData;
 */
apx_error_t apx_nodeInstance_updatePortDataDirectById(apx_nodeInstance_t *destNode, apx_portId_t destPortId, apx_nodeInstance_t *srcNode, apx_portId_t srcPortId)
{
   if ( (destNode != 0) && (destPortId >= 0) && (destPortId < destNode->nodeInfo->numRequirePorts) && (srcNode != 0) && (srcPortId >= 0) && (srcPortId < srcNode->nodeInfo->numProvidePorts) )
   {
      if ( (destNode->nodeData == 0) || (srcNode->nodeData == 0))
      {
         return APX_NULL_PTR_ERROR;
      }
      else
      {
         apx_portDataProps_t *destDataProps;
         apx_portDataProps_t *srcDataProps;
         destDataProps = apx_nodeInfo_getRequirePortDataProps(destNode->nodeInfo, destPortId);
         srcDataProps = apx_nodeInfo_getProvidePortDataProps(srcNode->nodeInfo, srcPortId);
         return apx_nodeInstance_updatePortDataDirect(destNode, destDataProps, srcNode, srcDataProps);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_buildNodeInfo(apx_nodeInstance_t *self, apx_programType_t *errProgramType, apx_uniquePortId_t *errPortId)
{
   if ( (self != 0) && (errProgramType != 0) && (errPortId != 0))
   {
      if (self->nodeInfo == 0)
      {
         self->nodeInfo = apx_nodeInfo_new();
         if (self->nodeInfo == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      if (self->parseTree != 0)
      {
         apx_error_t rc;
         apx_compiler_t compiler;
         apx_compiler_create(&compiler);
         assert(self->nodeInfo != 0);
         rc = apx_nodeInfo_build(self->nodeInfo, self->parseTree, &compiler, self->mode, errProgramType, errPortId);
         apx_compiler_destroy(&compiler);
         if (rc != APX_NO_ERROR)
         {
            apx_nodeInfo_delete(self->nodeInfo);
            self->nodeInfo = 0;
         }
         return rc;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_nodeInfo_t *apx_nodeInstane_getNodeInfo(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      return self->nodeInfo;
   }
   return (apx_nodeInfo_t*) 0;
}

/**
 * Builds port reference data structures in this nodeInstance.
 * This call is only allowed after successfully calling apx_nodeInstance_buildNodeInfo (It's depending on data generated inside nodeInfo)
 */
apx_error_t apx_nodeInstance_buildPortRefs(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      uint32_t numRequirePorts;
      uint32_t numProvidePorts;
      size_t allocSize;
      if (self->nodeInfo == 0)
      {
         return APX_NULL_PTR_ERROR;
      }
      numRequirePorts = apx_nodeInfo_getNumRequirePorts(self->nodeInfo);
      numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
      if (numRequirePorts > 0)
      {
         allocSize = numRequirePorts * sizeof(apx_portRef_t);
         self->requirePortReferences = (apx_portRef_t*) malloc(allocSize);
         if (self->requirePortReferences == 0)
         {
            return APX_MEM_ERROR;
         }
         else
         {
            apx_nodeInstance_initPortRefs(self, self->requirePortReferences, numRequirePorts, 0u, apx_nodeInfo_getRequirePortDataProps);
         }
      }
      if (numProvidePorts > 0)
      {
         allocSize = numProvidePorts * sizeof(apx_portRef_t);
         self->providePortReferences = (apx_portRef_t*) malloc(allocSize);
         if (self->providePortReferences == 0)
         {
            if (self->requirePortReferences != 0)
            {
               free(self->requirePortReferences);
               self->requirePortReferences = (apx_portRef_t*) 0;
            }
            return APX_MEM_ERROR;
         }
         else
         {
            apx_nodeInstance_initPortRefs(self, self->providePortReferences, numProvidePorts, APX_PORT_ID_PROVIDE_PORT,  apx_nodeInfo_getProvidePortDataProps);
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_portRef_t *apx_nodeInstance_getPortRef(apx_nodeInstance_t *self, apx_uniquePortId_t portId)
{
   if ( ((portId & APX_PORT_ID_PROVIDE_PORT) != 0u ))
   {
      return apx_nodeInstance_getProvidePortRef(self, portId & APX_PORT_ID_MASK);
   }
   return (apx_portRef_t*) 0;
}

apx_portRef_t *apx_nodeInstance_getRequirePortRef(apx_nodeInstance_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->nodeInfo != 0) && (self->requirePortReferences != 0) )
   {
      apx_portCount_t numRequirePorts = apx_nodeInfo_getNumRequirePorts(self->nodeInfo);
      if (portId >= 0 && portId < numRequirePorts)
      {
         return &self->requirePortReferences[portId];
      }
   }
   return (apx_portRef_t*) 0;
}

apx_portRef_t *apx_nodeInstance_getProvidePortRef(apx_nodeInstance_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->nodeInfo != 0) && (self->providePortReferences != 0) )
   {
      apx_portCount_t numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
      if (portId >= 0 && portId < numProvidePorts)
      {
         return &self->providePortReferences[portId];
      }
   }
   return (apx_portRef_t*) 0;
}

void apx_nodeInstance_registerDefinitionFileHandler(apx_nodeInstance_t *self, apx_file_t *file)
{
   if ( (self != 0) && (file != 0) )
   {
      apx_fileNotificationHandler_t handler = {0, 0, 0};
      handler.arg = (void*) self;
      if (apx_file_isRemoteFile(file))
      {
         handler.writeNotify = apx_nodeInstance_definitionFileWriteNotify;
      }
      else
      {
         handler.openNotify = apx_nodeInstance_definitionFileOpenNotify;
      }
      apx_file_setNotificationHandler(file, &handler);
      self->definitionFile = file;
   }
}

void apx_nodeInstance_registerProvidePortFileHandler(apx_nodeInstance_t *self, apx_file_t *file)
{
   if ( (self != 0) && (file != 0) )
   {
      apx_fileNotificationHandler_t handler = {0, 0, 0};
      handler.arg = (void*) self;
      if (apx_file_isRemoteFile(file))
      {
         handler.writeNotify = apx_nodeInstance_providePortDataFileWriteNotify;
      }
      else
      {
         handler.openNotify = apx_nodeInstance_providePortDataFileOpenNotify;
      }
      apx_file_setNotificationHandler(file, &handler);
      self->providePortDataFile = file;
   }
}

void apx_nodeInstance_registerRequirePortFileHandler(apx_nodeInstance_t *self, apx_file_t *file)
{
   if ( (self != 0) && (file != 0) )
   {
      apx_fileNotificationHandler_t handler = {0, 0, 0};
      handler.arg = (void*) self;
      if (apx_file_isRemoteFile(file))
      {
         handler.writeNotify = apx_nodeInstance_requirePortDataFileWriteNotify;
      }
      else
      {
         handler.openNotify = apx_nodeInstance_requirePortDataFileOpenNotify;
      }
      apx_file_setNotificationHandler(file, &handler);
      self->requirePortDataFile = file;
   }
}

const char *apx_nodeInstance_getName(apx_nodeInstance_t *self)
{
   if ( (self != 0) && (self->nodeInfo != 0) )
   {
      return apx_nodeInfo_getName(self->nodeInfo);
   }
   return (const char*) 0;
}

int32_t apx_nodeInstance_getNumProvidePorts(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      if (self->nodeInfo != 0)
      {
         return apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
      }
      else if (self->parseTree != 0)
      {
         return apx_node_getNumProvidePorts(self->parseTree);
      }
      else
      {
         //MISRA
      }
   }
   return -1;
}

int32_t apx_nodeInstance_getNumRequirePorts(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      if (self->nodeInfo != 0)
      {
         return apx_nodeInfo_getNumRequirePorts(self->nodeInfo);
      }
      else if (self->parseTree != 0)
      {
         return apx_node_getNumRequirePorts(self->parseTree);
      }
      else
      {
         //MISRA
      }
   }
   return -1;
}


apx_error_t apx_nodeInstance_fillProvidePortDataFileInfo(apx_nodeInstance_t *self, apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0))
   {
      if (self->nodeInfo != 0)
      {
         uint32_t fileSize;
         fileSize = (uint32_t) apx_nodeInfo_getProvidePortInitDataSize(self->nodeInfo);
         assert(fileSize > 0);
         return apx_nodeInstance_createFileInfo(self, APX_OUTDATA_FILE_EXT, fileSize, fileInfo);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_fillDefinitionFileInfo(apx_nodeInstance_t *self, apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0))
   {
      if (self->nodeData != 0)
      {
         uint32_t fileSize;
         fileSize = (uint32_t) apx_nodeData_getDefinitionDataLen(self->nodeData);
         assert(fileSize > 0);
         return apx_nodeInstance_createFileInfo(self, APX_DEFINITION_FILE_EXT, fileSize, fileInfo);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}



void apx_nodeInstance_setConnection(apx_nodeInstance_t *self, struct apx_connectionBase_tag *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      self->connection = connection;
   }
}

struct apx_connectionBase_tag* apx_nodeInstance_getConnection(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      return self->connection;
   }
   return (struct apx_connectionBase_tag*) 0;
}

void apx_nodeInstance_cleanParseTree(apx_nodeInstance_t *self)
{
   if ( (self != 0) && (self->parseTree != 0) )
   {
      apx_node_delete(self->parseTree);
      self->parseTree = (apx_node_t*) 0;
   }
}

apx_nodeInfo_t *apx_nodeInstance_getNodeInfo(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      return self->nodeInfo;
   }
   return (apx_nodeInfo_t*) 0;
}

void apx_nodeInstance_setProvidePortDataState(apx_nodeInstance_t *self, apx_providePortDataState_t state)
{
   if (self != 0)
   {
      //TODO: Introduce some form of thread-lock here
      self->providePortDataState = state;
   }
}

apx_providePortDataState_t apx_nodeInstance_getProvidePortDataState(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      //TODO: Introduce some form of thread-lock here
      return self->providePortDataState;
   }
   return APX_PROVIDE_PORT_DATE_STATE_INIT;
}

void apx_nodeInstance_setRequirePortDataState(apx_nodeInstance_t *self, apx_requirePortDataState_t state)
{
   if (self != 0)
   {
      //TODO: Introduce some form of thread-lock here
      self->requirePortDataState = state;
   }
}

apx_requirePortDataState_t apx_nodeInstance_getRequirePortDataState(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      //TODO: Introduce some form of thread-lock here
      return self->requirePortDataState;
   }
   return APX_REQUIRE_PORT_DATA_STATE_INIT;
}






/********** Data API  ************/

apx_error_t apx_nodeInstance_writeDefinitionData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (src != 0) )
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData_writeDefinitionData(self->nodeData, src, offset, len);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/*
apx_error_t apx_nodeInstance_readDefinitionData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (dest != 0) )
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData_readDefinitionData(self->nodeData, dest, offset, len);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
*/

/**
 * Updates ProvidePortData in this node instance.
 * If the node is currently connected it will also forward the new data to remote side
 */
apx_error_t apx_nodeInstance_writeProvidePortData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, apx_size_t len)
{
   if ( (self != 0) && (src != 0) )
   {
      if (self->nodeData != 0)
      {
         apx_error_t rc = apx_nodeData_writeProvidePortData(self->nodeData, src, offset, len);
         if (rc != APX_NO_ERROR)
         {
            return rc;
         }
         if(self->connection != 0)
         {
            assert(self->providePortDataFile != 0);
            rc = apx_connectionBase_updateProvidePortDataDirect(self->connection, self->providePortDataFile, src, offset, len);
         }
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_readProvidePortData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, apx_size_t len)
{
   if ( (self != 0) && (dest != 0) )
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData_readProvidePortData(self->nodeData, dest, offset, len);
      }
      else
      {
         return APX_NULL_PTR_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Reads raw data from requirePortData buffer
 */
apx_error_t apx_nodeInstance_readRequirePortData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (dest != 0) )
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData_readRequirePortData(self->nodeData, dest, offset, len);
      }
      else
      {
         return APX_NULL_PTR_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_writeRequirePortData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, apx_size_t len)
{
   if ( (self != 0) && (src != 0) )
   {
      assert(self->nodeData != 0);

      apx_error_t rc = apx_nodeData_writeRequirePortData(self->nodeData, src, offset, len);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if(self->connection != 0)
      {
         assert(self->requirePortDataFile != 0);
         if (self->mode == APX_SERVER_MODE)
         {
            rc = apx_connectionBase_updateRequirePortDataDirect(self->connection, self->requirePortDataFile, src, offset, len);
         }
      }
      return rc;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/********** P-Port connector API  ************/
apx_error_t apx_nodeInstance_buildConnectorTable(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      apx_portCount_t numProvidePorts;
      assert(self->nodeInfo != 0);
      numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
      if (numProvidePorts > 0)
      {
         apx_portId_t portId;
         size_t allocSize = numProvidePorts * sizeof(apx_portConnectorList_t);
         self->connectorTable = (apx_portConnectorList_t*) malloc(allocSize);
         MUTEX_LOCK(self->connectorTableLock);
         for(portId = 0; portId < numProvidePorts; portId++)
         {
            apx_portConnectorList_create(&self->connectorTable[portId]);
         }
         MUTEX_UNLOCK(self->connectorTableLock);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_nodeInstance_lockPortConnectorTable(apx_nodeInstance_t *self)
{
   if ( (self != 0) && (self->connectorTable != 0) )
   {
      MUTEX_LOCK(self->connectorTableLock);
   }
}

void apx_nodeInstance_unlockPortConnectorTable(apx_nodeInstance_t *self)
{
   if ( (self != 0) && (self->connectorTable != 0) )
   {
      MUTEX_UNLOCK(self->connectorTableLock);
   }
}

apx_portConnectorList_t *apx_nodeInstance_getProvidePortConnectors(apx_nodeInstance_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (self->connectorTable != 0) )
   {
      {
         apx_portCount_t numProvidePorts;
         assert(self->nodeInfo != 0);
         numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
         if ( (portId >= 0) && (portId < numProvidePorts) )
         {
            return &self->connectorTable[portId];
         }
      }
   }
   return (apx_portConnectorList_t*) 0;
}

/**
 * Creates a new connector from this nodes' P-Port to another nodes' R-port.
 * The caller of this function must have previously have called apx_nodeInstance_lockPortConnectorTable on this object
 */
apx_error_t apx_nodeInstance_insertProvidePortConnector(apx_nodeInstance_t *self, apx_portId_t portId, apx_portRef_t *requirePortRef)
{
   if ( (self != 0) && (portId >= 0) && (requirePortRef != 0) )
   {
      apx_portCount_t numProvidePorts;
      assert(self->nodeInfo != 0);
      numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
      if (portId < numProvidePorts)
      {
         if (self->connectorTable != 0)
         {
            apx_portConnectorList_t *connectors = &self->connectorTable[portId];
            return apx_portConnectorList_insert(connectors, requirePortRef);
         }
         return APX_NULL_PTR_ERROR;
      }
      //fall-through to return APX_INVALID_ARGUMENT_ERROR
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Undo action previously performed by apx_nodeInstance_insertProvidePortConnector.
 * The caller must have previously called apx_nodeInstance_lockPortConnectorTable on this object
 */
apx_error_t apx_nodeInstance_removeProvidePortConnector(apx_nodeInstance_t *self, apx_portId_t portId, apx_portRef_t *requirePortRef)
{
   if ( (self != 0) && (portId >= 0) && (requirePortRef != 0) )
   {
      apx_portCount_t numProvidePorts;
      assert(self->nodeInfo != 0);
      numProvidePorts = apx_nodeInfo_getNumProvidePorts(self->nodeInfo);
      if (portId < numProvidePorts)
      {
         if (self->connectorTable != 0)
         {
            apx_portConnectorList_t *connectors = &self->connectorTable[portId];
            apx_portConnectorList_remove(connectors, requirePortRef);
            return APX_NO_ERROR;
         }
         return APX_NULL_PTR_ERROR;
      }
      //fall-through to return APX_INVALID_ARGUMENT_ERROR
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/********** Port Connection Changes API  ************/
apx_portConnectorChangeTable_t* apx_nodeInstance_getRequirePortConnectorChanges(apx_nodeInstance_t *self, bool autoCreate)
{
   if (self != 0)
   {
      if ( (self->requirePortChanges == 0) && (autoCreate) )
      {
         assert(self->nodeInfo != 0);
         self->requirePortChanges = apx_portConnectorChangeTable_new(apx_nodeInfo_getNumRequirePorts(self->nodeInfo));
         if (self->connection != 0)
         {
            apx_connectionBase_portConnectorChangeCreateNotify(self->connection, self, APX_REQUIRE_PORT);
         }
      }
      return self->requirePortChanges;
   }
   return (apx_portConnectorChangeTable_t*) 0;
}

apx_portConnectorChangeTable_t* apx_nodeInstance_getProvidePortConnectorChanges(apx_nodeInstance_t *self, bool autoCreate)
{
   {
      if (self != 0)
      {
         if ( (self->providePortChanges == 0) && (autoCreate))
         {
            assert(self->nodeInfo != 0);
            self->providePortChanges = apx_portConnectorChangeTable_new(apx_nodeInfo_getNumProvidePorts(self->nodeInfo));
            if (self->connection != 0)
            {
               apx_connectionBase_portConnectorChangeCreateNotify(self->connection, self, APX_PROVIDE_PORT);
            }
         }
         return self->providePortChanges;
      }
      return (apx_portConnectorChangeTable_t*) 0;
   }
}

/**
 * This clears the internal pointer. This implicitly means the caller of this function has now taken ownership of
 * the data structure and is now responsible for its memory management.
 */
void apx_nodeInstance_clearRequirePortConnectorChanges(apx_nodeInstance_t *self, bool releaseMemory)
{
   if (self != 0)
   {
      if (releaseMemory && (self->requirePortChanges != 0))
      {
         apx_portConnectorChangeTable_delete(self->requirePortChanges);
      }
      self->requirePortChanges = (apx_portConnectorChangeTable_t*) 0;
   }
}

/**
 * This clears the internal pointer. This implicitly means the caller of this function has now taken ownership of
 * the data structure and is now responsible for its memory management.
 */
void apx_nodeInstance_clearProvidePortConnectorChanges(apx_nodeInstance_t *self, bool releaseMemory)
{
   if (self != 0)
   {
      if (releaseMemory && (self->providePortChanges != 0))
      {
         apx_portConnectorChangeTable_delete(self->providePortChanges);
      }
      self->providePortChanges = (apx_portConnectorChangeTable_t*) 0;
   }
}

apx_error_t apx_nodeInstance_handleRequirePortDataDisconnected(apx_nodeInstance_t *self, apx_portConnectorChangeTable_t *connectorChanges)
{
   if ( (self != 0) && (connectorChanges != 0) )
   {
      apx_portId_t requirePortId;
      apx_portCount_t numRequirePorts = apx_nodeInstance_getNumRequirePorts(self);
      for (requirePortId = 0; requirePortId < numRequirePorts; requirePortId++)
      {
         apx_portRef_t *requirePortRef;
         apx_portConnectorChangeEntry_t *entry = apx_portConnectorChangeTable_getEntry(connectorChanges, requirePortId);
         requirePortRef = apx_nodeInstance_getRequirePortRef(self, requirePortId);
         if (entry->count == -1)
         {
            apx_error_t rc;
            apx_portId_t providePortId;
            apx_nodeInstance_t *provideNodeInstance;
            apx_portRef_t *providePortRef = entry->data.portRef;
            provideNodeInstance = providePortRef->nodeInstance;
            assert(provideNodeInstance != 0);
            providePortId = apx_portRef_getPortId(providePortRef);
            apx_nodeInstance_lockPortConnectorTable(provideNodeInstance);
            rc = apx_nodeInstance_removeProvidePortConnector(provideNodeInstance, providePortId, requirePortRef);
            apx_nodeInstance_unlockPortConnectorTable(provideNodeInstance);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
         }
         else if (entry->count < -1)
         {
            //TODO: Handle multiple providers
            return APX_NOT_IMPLEMENTED_ERROR;
         }
         else
         {
            assert(entry->count == 0);
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/********** Data Routing API  ************/

/**
 * Call once per port when new RequirePortData was just connected to a provide port
 *
 */
apx_error_t apx_nodeInstance_handleRequirePortWasConnectedToProvidePort(apx_portRef_t *requirePortRef, apx_portRef_t *providePortRef)
{
   if ( (requirePortRef != 0) && (providePortRef != 0) )
   {
      apx_error_t rc;
      apx_nodeInstance_t *requireNodeInstance;
      apx_nodeInstance_t *provideNodeInstance;
      apx_portId_t providePortId;
      requireNodeInstance = requirePortRef->nodeInstance;
      provideNodeInstance = providePortRef->nodeInstance;
      providePortId = apx_portRef_getPortId(providePortRef);
      assert(requireNodeInstance != 0);
      assert(provideNodeInstance != 0);
      assert(providePortId >= 0u);
      apx_nodeInstance_lockPortConnectorTable(provideNodeInstance);
      rc = apx_nodeInstance_insertProvidePortConnector(provideNodeInstance, providePortId, requirePortRef);
      apx_nodeInstance_unlockPortConnectorTable(provideNodeInstance);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      //Copy init-value for R-PORT directly from provide port connector
      rc = apx_nodeInstance_updatePortDataDirect(requireNodeInstance, requirePortRef->portDataProps,
            provideNodeInstance, providePortRef->portDataProps);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_handleProvidePortWasConnectedToRequirePort(apx_portRef_t *providePortRef, apx_portRef_t *requirePortRef)
{
   if ( (requirePortRef != 0) && (providePortRef != 0) )
   {
      apx_error_t rc;
      apx_nodeInstance_t *provideNodeInstance;
      provideNodeInstance = providePortRef->nodeInstance;
      apx_portId_t providePortId = apx_portRef_getPortId(providePortRef);
      assert(provideNodeInstance != 0);
      assert(providePortId >= 0u);
      rc = apx_nodeInstance_insertProvidePortConnector(provideNodeInstance, providePortId, requirePortRef);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      rc = apx_nodeInstance_routeProvidePortDataToRequirePortByRef(providePortRef, requirePortRef);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_handleRequirePortWasDisconnectedFromProvidePort(apx_portRef_t *requirePortRef, apx_portRef_t *providePortRef)
{
   if ( (requirePortRef != 0) && (providePortRef != 0) )
   {

      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_sendRequirePortDataToFileManager(apx_nodeInstance_t *self)
{
   if ( self != 0 )
   {
      uint8_t *dataBuf;
      size_t bufSize;
      apx_size_t fileSize;
      uint32_t fileStartAddress;
      apx_error_t rc;
      apx_file_t *file;
      apx_fileManager_t *fileManager;
      file = self->requirePortDataFile;
      assert(file != 0);
      fileManager = apx_file_getFileManager(file);
      assert(fileManager != 0);
      assert(self->nodeData != 0);
      if (self->connection == 0)
      {
         return APX_NOT_CONNECTED_ERROR;
      }
      bufSize = (size_t) apx_nodeData_getRequirePortDataLen(self->nodeData);
      fileSize = apx_file_getFileSize(file);
      fileStartAddress = apx_file_getStartAddress(file);
      assert(fileSize > 0);
      if (fileSize != bufSize)
      {
         if (bufSize == 0u)
         {
            return APX_MISSING_BUFFER_ERROR;
         }
         else
         {
            return APX_LENGTH_ERROR;
         }
      }
      if (fileSize > APX_MAX_FILE_SIZE)
      {
         return APX_FILE_TOO_LARGE_ERROR;
      }
      dataBuf = apx_connectionBase_alloc(self->connection, bufSize);
      if (dataBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      rc = apx_nodeData_readRequirePortData(self->nodeData, dataBuf, 0u, bufSize);
      if (rc != APX_NO_ERROR)
      {
         apx_connectionBase_free(self->connection, dataBuf, bufSize);
      }
      return apx_fileManager_writeDynamicData(fileManager, fileStartAddress, fileSize, dataBuf);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_routeProvidePortDataToReceivers(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, apx_size_t len)
{
   if ( (self != 0) )
   {
      uint32_t endOffset;
      assert(self->nodeInfo != 0);
      assert(self->connectorTable != 0);
      endOffset = offset + len;
      MUTEX_LOCK(self->connectorTableLock);
      while(offset < endOffset)
      {
         apx_error_t rc;
         int32_t numConnectors;
         int32_t connectorId;
         apx_portId_t providerPortId;
         apx_portConnectorList_t *portConnectors;
         const apx_portDataProps_t *providePortDataProps;
         providerPortId = apx_nodeInfo_findProvidePortIdFromByteOffset(self->nodeInfo, offset);
         if (providerPortId < 0)
         {
            printf("[APX_NODEINSTANCE] detected write on invalid offset %d\n", offset);
            break;
         }
         providePortDataProps = apx_nodeInfo_getProvidePortDataProps(self->nodeInfo, providerPortId);
         assert(providePortDataProps != 0);
         offset += providePortDataProps->dataSize; ///TODO: Verify if this also works for complex data
         portConnectors = &self->connectorTable[providerPortId];
         numConnectors = apx_portConnectorList_length(portConnectors);
         for(connectorId = 0; connectorId < numConnectors; connectorId++)
         {
            const apx_portDataProps_t *requireePortDataProps;
            apx_portRef_t *requirePortRef = apx_portConnectorList_get(portConnectors, connectorId);
            requireePortDataProps = requirePortRef->portDataProps;
            if (apx_portDataProps_isPlainOldData(requireePortDataProps))
            {
               if (requireePortDataProps->dataSize != len)
               {
                  MUTEX_UNLOCK(self->connectorTableLock);
                  return APX_LENGTH_ERROR;
               }
               rc = apx_nodeInstance_writeRequirePortData(requirePortRef->nodeInstance, src, requireePortDataProps->offset, requireePortDataProps->dataSize);
               if (rc != APX_NO_ERROR)
               {
                  MUTEX_UNLOCK(self->connectorTableLock);
                  return rc;
               }
            }
         }
      }

      MUTEX_UNLOCK(self->connectorTableLock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_nodeInstance_clearConnectorTable(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      MUTEX_LOCK(self->connectorTableLock);
      apx_portConnectorList_clear(self->connectorTable);
      MUTEX_UNLOCK(self->connectorTableLock);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


static apx_error_t apx_nodeInstance_definitionFileWriteNotify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len)
{
   (void) file;
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if (self != 0)
   {
      apx_error_t retval;
#if APX_DEBUG_ENABLE
      //printf("definitionFileWriteNotify(%d, %d)\n", (int) offset, (int) len);
#endif
      //It's OK for definition data to be written directly by the node instance before notification
      retval = apx_nodeData_writeDefinitionData(self->nodeData, src, offset, len);
      if ( (self->connection != 0) && (retval == APX_NO_ERROR) )
      {
         retval = apx_connectionBase_nodeInstanceFileWriteNotify(self->connection, self, APX_DEFINITION_FILE_TYPE, offset, src, len);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#include <stdio.h>

static apx_error_t apx_nodeInstance_definitionFileOpenNotify(void *arg, struct apx_file_tag *file)
{
   apx_nodeInstance_t *self;
   self = (apx_nodeInstance_t*) arg;
   if (self != 0)
   {
      if (self->definitionFile == 0)
      {
         self->definitionFile = file;
      }
      assert(file->fileManager != 0);
      return apx_fileManager_writeConstData(apx_file_getFileManager(file), apx_file_getStartAddress(file), apx_file_getFileSize(file), apx_nodeInstance_definitionFileReadData, (void*) self);
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_nodeInstance_definitionFileReadData(void *arg, apx_file_t*file, uint32_t offset, uint8_t *dest, uint32_t len)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if (self != 0)
   {
      apx_nodeData_t *nodeData = apx_nodeInstance_getNodeData(self);
      if (nodeData != 0)
      {
         return apx_nodeData_readDefinitionData(nodeData, dest, offset, len);
      }
      else
      {
         return APX_NULL_PTR_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


static apx_error_t apx_nodeInstance_createFileInfo(apx_nodeInstance_t *self, const char *fileExtension, uint32_t fileSize, apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0))
   {
      char fileName[RMF_MAX_FILE_NAME+1];
      const char *nodeName;
      nodeName = apx_nodeInstance_getName(self);
      if (nodeName == 0)
      {
         return APX_NAME_MISSING_ERROR;
      }
      if ( (strlen(nodeName) + APX_MAX_FILE_EXT_LEN) >= sizeof(fileName) )
      {
         return APX_NAME_TOO_LONG_ERROR;
      }
      strcpy(fileName, nodeName);
      strcat(fileName, fileExtension);
      return apx_fileInfo_create(fileInfo, RMF_INVALID_ADDRESS, fileSize, fileName, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, (const uint8_t*) 0);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_nodeInstance_providePortDataFileWriteNotify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if ( (self != 0) && (file != 0) )
   {
      if (self->connection != 0)
      {
         //It's not OK for nodeInstance to manipulate it's own data without acquiring a global lock (either owned by server or client).
         //Forward this notification to parent connection to take correct action. The connection must write the data using the apx_nodeData API later.
         return apx_connectionBase_nodeInstanceFileWriteNotify(self->connection, self, apx_file_getApxFileType(file), offset, src, len);
      }
      else
      {
         return APX_NULL_PTR_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Takes a snapshot of the providePortData buffer and sends it away to the file manager
 */
static apx_error_t apx_nodeInstance_providePortDataFileOpenNotify(void *arg, struct apx_file_tag *file)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if ( (self != 0) && (file != 0) )
   {
      uint8_t *dataBuf;
      size_t bufSize;
      apx_size_t fileSize;
      uint32_t fileStartAddress;
      apx_error_t rc;
      apx_fileManager_t *fileManager = apx_file_getFileManager(file);
      assert(fileManager != 0);
      assert(self->nodeData != 0);
      assert(self->providePortDataFile != 0);
      if (self->connection == 0)
      {
         return APX_NULL_PTR_ERROR;
      }
      bufSize = (size_t) apx_nodeData_getProvidePortDataLen(self->nodeData);
      fileSize = apx_file_getFileSize(file);
      fileStartAddress = apx_file_getStartAddress(file);
      assert(fileSize > 0);
      if (fileSize != bufSize)
      {
         if (bufSize == 0u)
         {
            return APX_MISSING_BUFFER_ERROR;
         }
         else
         {
            return APX_LENGTH_ERROR;
         }
      }
      if (fileSize > APX_MAX_FILE_SIZE)
      {
         return APX_FILE_TOO_LARGE_ERROR;
      }
      dataBuf = apx_connectionBase_alloc(self->connection, bufSize);
      if (dataBuf == 0)
      {
         return APX_MEM_ERROR;
      }
      rc = apx_nodeData_readProvidePortData(self->nodeData, dataBuf, 0u, bufSize);
      if (rc != APX_NO_ERROR)
      {
         apx_connectionBase_free(self->connection, dataBuf, bufSize);
      }
      return apx_fileManager_writeDynamicData(fileManager, fileStartAddress, fileSize, dataBuf);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_nodeInstance_requirePortDataFileWriteNotify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if ( (self != 0) && (file != 0) )
   {
      if (self->connection != 0)
      {
         return apx_connectionBase_nodeInstanceFileWriteNotify(self->connection, self, apx_file_getApxFileType(file), offset, src, len);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_nodeInstance_requirePortDataFileOpenNotify(void *arg, struct apx_file_tag *file)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if ( (self != 0) && (file != 0) )
   {
      if (self->connection != 0)
      {
         return apx_connectionBase_nodeInstanceFileOpenNotify(self->connection, self, apx_file_getApxFileType(file));
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


static void apx_nodeInstance_initPortRefs(apx_nodeInstance_t *self, apx_portRef_t *portRefs, apx_portCount_t numPorts, uint32_t portIdMask, apx_getPortDataPropsFunc *getPortDataProps)
{
   apx_portId_t portId;
   assert(portRefs != 0);
   assert(self != 0);
   assert(self->nodeInfo != 0);
   assert(getPortDataProps != 0);
   for (portId=0; portId < numPorts; portId++)
   {
      const apx_portDataProps_t *dataProps = getPortDataProps(self->nodeInfo, portId);
      assert(dataProps != 0);
      apx_portRef_create(&portRefs[portId], self,  (portId|portIdMask), dataProps);
   }
}

static apx_error_t apx_nodeInstance_routeProvidePortDataToRequirePortByRef(apx_portRef_t *providePortRef, apx_portRef_t *requirePortRef)
{
   assert(providePortRef != 0);
   assert(requirePortRef != 0);
   const apx_portDataProps_t *requirePortDataProps;
   const apx_portDataProps_t *providePortDataProps;
   requirePortDataProps = requirePortRef->portDataProps;
   providePortDataProps = providePortRef->portDataProps;
   if ( apx_portDataProps_isPlainOldData(requirePortDataProps) )
   {
      apx_nodeInstance_t *provideNodeInstance;
      apx_nodeInstance_t *requireNodeInstance;
      apx_connectionBase_t *requireConnection;
      assert(providePortDataProps->dataSize == requirePortDataProps->dataSize);
      provideNodeInstance = providePortRef->nodeInstance;
      requireNodeInstance = requirePortRef->nodeInstance;
      requireConnection = apx_nodeInstance_getConnection(requireNodeInstance);
      if ( (requireConnection != 0) && (requireNodeInstance->requirePortDataFile) )
      {
         uint8_t stackDataBuf[STACK_DATA_BUF_SIZE];
         uint8_t *providePortDataBuf = &stackDataBuf[0];
         bool isDataBufMalloced;
         apx_error_t rc;
         isDataBufMalloced = providePortDataProps->dataSize > STACK_DATA_BUF_SIZE;
         if (isDataBufMalloced)
         {
            providePortDataBuf = (uint8_t*) malloc(providePortDataProps->dataSize);
            if (providePortDataBuf == NULL)
            {
               return APX_MEM_ERROR;
            }
         }
         rc = apx_nodeInstance_readProvidePortData(provideNodeInstance, providePortDataBuf, providePortDataProps->offset, providePortDataProps->dataSize);
         if (rc != APX_NO_ERROR)
         {
            if (isDataBufMalloced) free(providePortDataBuf);
            return rc;
         }
         rc = apx_connectionBase_updateRequirePortDataDirect(requireConnection,
               requireNodeInstance->requirePortDataFile,
               providePortDataBuf,
               requirePortDataProps->offset,
               requirePortDataProps->dataSize);
         if (rc != APX_NO_ERROR)
         {
            if (isDataBufMalloced) free(providePortDataBuf);
            return rc;
         }
      }
      else
      {
         return APX_NULL_PTR_ERROR;
      }
   }
   else
   {
      return APX_NOT_IMPLEMENTED_ERROR;
   }
   return APX_NO_ERROR;
}

