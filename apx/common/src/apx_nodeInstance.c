/*****************************************************************************
* \file      apx_nodeInstance.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Parent container for all things node-related.
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
#include <assert.h>
#include "apx_nodeInstance.h"
#include "apx_connectionBase.h"
#include "rmf.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_nodeInstance_writeDefinitionFileHandler(void *arg, apx_file2_t *file, uint32_t offset, const uint8_t *src, uint32_t len, bool more);
static apx_error_t apx_nodeInstance_readDefinitionFileHandler(void *arg, struct apx_file2_tag *file, uint32_t offset, uint8_t *dest, uint32_t len);
static apx_error_t apx_nodeInstance_writeProvidePortDataFileHandler(void *arg, apx_file2_t *file, uint32_t offset, const uint8_t *src, uint32_t len, bool more);
static apx_error_t apx_nodeInstance_readProvidePortDataFileHandler(void *arg, struct apx_file2_tag *file, uint32_t offset, uint8_t *dest, uint32_t len);
static apx_error_t apx_nodeInstance_writeRequirePortDataFileHandler(void *arg, apx_file2_t *file, uint32_t offset, const uint8_t *src, uint32_t len, bool more);
static apx_error_t apx_nodeInstance_readRequirePortDataFileHandler(void *arg, struct apx_file2_tag *file, uint32_t offset, uint8_t *dest, uint32_t len);
static apx_error_t apx_nodeInstance_createFileInfo(apx_nodeInstance_t *self, const char *fileExtension, uint32_t fileSize, apx_fileInfo_t *fileInfo);

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
      if (self->nodeInfo != 0)
      {
         apx_nodeInfo_delete(self->nodeInfo);
      }
      if (self->nodeData != 0)
      {
         apx_nodeData2_delete(self->nodeData);
      }
      if (self->requirePortDataRef != 0)
      {
         apx_portDataRef2_delete(self->requirePortDataRef);
      }
      if (self->providePortDataRef != 0)
      {
         apx_portDataRef2_delete(self->providePortDataRef);
      }
      if (self->portTriggerList != 0)
      {
         apx_portTriggerList_delete(self->portTriggerList);
      }
   }
}

apx_nodeInstance_t *apx_nodeInstance_new(apx_mode_t mode)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) malloc(sizeof(apx_nodeInstance_t));
   if(self != 0)
   {
      apx_nodeInstance_create(self, mode);
      apx_nodeData2_t *nodeData = apx_nodeInstance_createNodeData(self);
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

apx_nodeData2_t* apx_nodeInstance_createNodeData(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      if (self->nodeData == 0)
      {
         self->nodeData = apx_nodeData2_new();
      }
      return self->nodeData;
   }
   return (apx_nodeData2_t*) 0;
}

apx_nodeData2_t* apx_nodeInstance_getNodeData(apx_nodeInstance_t *self)
{
   if (self != 0)
   {
      return self->nodeData;
   }
   return (apx_nodeData2_t*) 0;
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
         apx_size_t definitionLen = apx_nodeData2_getDefinitionDataLen(self->nodeData);
         if (definitionLen > 0)
         {
            apx_node_t *parseTree;
            apx_nodeData2_lockDefinitionData(self->nodeData);
            parseTree = apx_parser_parseBuffer(parser, apx_nodeData2_getDefinitionDataBuf(self->nodeData), definitionLen);
            apx_nodeData2_unlockDefinitionData(self->nodeData);
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
         return apx_nodeData2_createDefinitionBuffer(self->nodeData, bufferLen);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}




apx_error_t apx_nodeInstance_updatePortDataDirect(apx_nodeInstance_t *destNode, apx_portDataProps_t *destDataProps, apx_nodeInstance_t *srcNode, apx_portDataProps_t *srcDataProps)
{
   return APX_NOT_IMPLEMENTED_ERROR;
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

void apx_nodeInstance_initDefinitionFileHandler(apx_nodeInstance_t *self, apx_file2_t *file, bool readonly)
{
   if (self != 0)
   {
      apx_file_handler_t handler;
      handler.arg = (void*) self;
      handler.read = apx_nodeInstance_readDefinitionFileHandler;
      handler.write = readonly? (apx_file_write_func*) 0 : apx_nodeInstance_writeDefinitionFileHandler;
      apx_file2_setHandler(file, &handler);
      self->definitionFile = file;
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


apx_error_t apx_nodeInstance_createProvidePortDataFileInfo(apx_nodeInstance_t *self, apx_fileInfo_t *fileInfo)
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

apx_error_t apx_nodeInstance_createDefinitionFileInfo(apx_nodeInstance_t *self, apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0))
   {
      if (self->nodeData != 0)
      {
         uint32_t fileSize;
         fileSize = (uint32_t) apx_nodeData2_getDefinitionDataLen(self->nodeData);
         assert(fileSize > 0);
         return apx_nodeInstance_createFileInfo(self, APX_DEFINITION_FILE_EXT, fileSize, fileInfo);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_nodeInstance_writeDefinitionData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (src != 0) )
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData2_writeDefinitionData(self->nodeData, src, offset, len);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_readDefinitionData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (dest != 0) )
   {
      if (self->nodeData != 0)
      {
         return apx_nodeData2_readDefinitionData(self->nodeData, dest, offset, len);
      }
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

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_nodeInstance_writeDefinitionFileHandler(void *arg, apx_file2_t *file, uint32_t offset, const uint8_t *src, uint32_t len, bool more)
{
   (void) file;
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if (self != 0)
   {
      return apx_nodeData2_writeDefinitionData(self->nodeData, src, offset, len);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_nodeInstance_readDefinitionFileHandler(void *arg, struct apx_file2_tag *file, uint32_t offset, uint8_t *dest, uint32_t len)
{
   (void) file;
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) arg;
   if (self != 0)
   {
      return apx_nodeData2_readDefinitionData(self->nodeData, dest, offset, len);
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
