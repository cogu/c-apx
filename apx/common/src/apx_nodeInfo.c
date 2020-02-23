/*****************************************************************************
* \file      apx_nodeInfo.c
* \author    Conny Gustafsson
* \date      2019-11-25
* \brief     Static information about an APX node.
*            This a post-processed version based on information from the APX definition parse tree.
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
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx_node.h"
#include "apx_parser.h"
#include "apx_nodeInfo.h"
#include "apx_vm.h"
#include <stdio.h> //DEBUG ONLY
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_nodeInfo_allocateMemory(apx_nodeInfo_t *self);
static void apx_nodeInfo_freeMemory(apx_nodeInfo_t *self);
static void apx_nodeInfo_createRequirePortDataProps(apx_nodeInfo_t *self);
static void apx_nodeInfo_createProvidePortDataProps(apx_nodeInfo_t *self);
static apx_error_t apx_nodeInfo_initClientBytePortMap(apx_nodeInfo_t *self);
static apx_error_t apx_nodeInfo_initServerBytePortMap(apx_nodeInfo_t *self);
static apx_error_t apx_nodeInfo_compilePortPrograms(apx_nodeInfo_t *self, apx_compiler_t *compiler, const apx_node_t *node, apx_programType_t *errProgramType, apx_uniquePortId_t *errPortId);
static apx_error_t apx_nodeInfo_createRequirePortInitData(apx_nodeInfo_t *self, const apx_node_t *node);
static apx_error_t apx_nodeInfo_createProvidePortInitData(apx_nodeInfo_t *self, const apx_node_t *node);
static uint8_t* apx_nodeInfo_createInitDataBuf(apx_size_t dataSize, adt_bytes_t **packPrograms, const adt_ary_t *ports, apx_portCount_t numPorts, apx_error_t *errorCode);
static apx_error_t apx_nodeInfo_buildRequirePortSignatures(apx_nodeInfo_t *self, const apx_node_t *node);
static apx_error_t apx_nodeInfo_buildProvidePortSignatures(apx_nodeInfo_t *self, const apx_node_t *node);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_nodeInfo_create(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      memset(self, 0, sizeof(apx_nodeInfo_t));
   }
}

void apx_nodeInfo_destroy(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      apx_nodeInfo_freeMemory(self);
   }
}

apx_nodeInfo_t *apx_nodeInfo_new(void)
{
   apx_nodeInfo_t *self = (apx_nodeInfo_t*) malloc(sizeof(apx_nodeInfo_t));
   if (self != 0)
   {
      apx_nodeInfo_create(self);
   }
   return self;
}

void apx_nodeInfo_delete(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      apx_nodeInfo_destroy(self);
      free(self);
   }
}

apx_error_t apx_nodeInfo_build(apx_nodeInfo_t *self, const struct apx_node_tag *parseTree, apx_compiler_t *compiler, apx_mode_t mode, apx_programType_t *errProgramType, apx_uniquePortId_t *errPortId)
{
   if ( (self != 0) && (parseTree != 0) && (compiler != 0) && ( (mode == APX_CLIENT_MODE) || (mode == APX_SERVER_MODE) ) )
   {
      apx_error_t errorCode;
      const char *nodeName;
      self->mode = mode;
      self->numRequirePorts = adt_ary_length(&parseTree->requirePortList);
      self->numProvidePorts = adt_ary_length(&parseTree->providePortList);
      nodeName = apx_node_getName(parseTree);
      if (nodeName != 0)
      {
         self->name = STRDUP(nodeName);
      }
      errorCode = apx_nodeInfo_allocateMemory(self);
      if (errorCode != APX_NO_ERROR)
      {
         apx_nodeInfo_freeMemory(self);
         return errorCode;
      }
      errorCode = apx_nodeInfo_compilePortPrograms(self, compiler, parseTree, errProgramType, errPortId);
      if (errorCode != APX_NO_ERROR)
      {
         apx_nodeInfo_freeMemory(self);
         return errorCode;
      }
      apx_nodeInfo_createRequirePortDataProps(self);
      apx_nodeInfo_createProvidePortDataProps(self);
      if(mode == APX_CLIENT_MODE)
      {
         errorCode = apx_nodeInfo_initClientBytePortMap(self);
      }
      else
      {
         errorCode = apx_nodeInfo_initServerBytePortMap(self);
      }
      if (errorCode != APX_NO_ERROR)
      {
         apx_nodeInfo_freeMemory(self);
         return errorCode;
      }
      if (self->numRequirePorts > 0)
      {
         errorCode = apx_nodeInfo_createRequirePortInitData(self, parseTree);
         if (errorCode != 0)
         {
            apx_nodeInfo_freeMemory(self);
            return errorCode;
         }
         if (mode == APX_SERVER_MODE)
         {
            errorCode = apx_nodeInfo_buildRequirePortSignatures(self, parseTree);
            if (errorCode != 0)
            {
               apx_nodeInfo_freeMemory(self);
               return errorCode;
            }
         }
      }
      if (self->numProvidePorts > 0)
      {
         errorCode = apx_nodeInfo_createProvidePortInitData(self, parseTree);
         if (errorCode != 0)
         {
            apx_nodeInfo_freeMemory(self);
            return errorCode;
         }
         if (mode == APX_SERVER_MODE)
         {
            errorCode = apx_nodeInfo_buildProvidePortSignatures(self, parseTree);
            if (errorCode != 0)
            {
               apx_nodeInfo_freeMemory(self);
               return errorCode;
            }
         }
      }
      self->requirePortDataLen = apx_nodeInfo_calcRequirePortDataLen(self);
      self->providePortDataLen = apx_nodeInfo_calcProvidePortDataLen(self);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_nodeInfo_t *apx_nodeInfo_make_from_cstr(const char *apx_definition, apx_mode_t mode)
{
   apx_nodeInfo_t *self = (apx_nodeInfo_t*) 0;
   apx_parser_t *parser = apx_parser_new();
   if (parser != 0)
   {
      apx_node_t *node = apx_parser_parseString(parser, apx_definition);
      if (node != 0)
      {
         self = apx_nodeInfo_new();
         if (self != 0)
         {
            apx_compiler_t *compiler = apx_compiler_new();
            if (compiler != 0)
            {
               apx_programType_t errProgramType = 0;
               apx_uniquePortId_t errPortId = 0;
               apx_error_t rc = apx_nodeInfo_build(self, node, compiler, mode, &errProgramType, &errPortId);
               if (rc != APX_NO_ERROR)
               {
                  apx_nodeInfo_delete(self);
                  self = (apx_nodeInfo_t*) 0;
               }
               apx_compiler_delete(compiler);
            }
            else
            {
               apx_nodeInfo_delete(self);
               self = (apx_nodeInfo_t*) 0;
            }
         }
      }
      apx_parser_delete(parser);
   }
   return self;
}

const char *apx_nodeInfo_getName(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->name;
   }
   return (const char*) 0;
}

apx_portCount_t apx_nodeInfo_getNumRequirePorts(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->numRequirePorts;
   }
   return -1;
}

apx_portCount_t apx_nodeInfo_getNumProvidePorts(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->numProvidePorts;
   }
   return -1;
}

apx_portDataProps_t *apx_nodeInfo_getRequirePortDataProps(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numRequirePorts))
   {
      return &self->requirePortDataProps[portId];
   }
   return (apx_portDataProps_t*) 0;
}

apx_portDataProps_t *apx_nodeInfo_getProvidePortDataProps(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numProvidePorts))
   {
      return &self->providePortDataProps[portId];
   }
   return (apx_portDataProps_t*) 0;
}

const adt_bytes_t* apx_nodeInfo_getRequirePortPackProgram(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId < self->numRequirePorts) && (self->requirePortPackPrograms != 0) )
   {
      return self->requirePortPackPrograms[portId];
   }
   return (const adt_bytes_t*) 0;
}

const adt_bytes_t* apx_nodeInfo_getProvidePortPackProgram(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId < self->numProvidePorts) && (self->providePortPackPrograms != 0) )
   {
      return self->providePortPackPrograms[portId];
   }
   return (const adt_bytes_t*) 0;
}

const adt_bytes_t* apx_nodeInfo_getRequirePortUnpackProgram(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId < self->numRequirePorts) && (self->requirePortUnpackPrograms != 0) )
   {
      return self->requirePortUnpackPrograms[portId];
   }
   return (const adt_bytes_t*) 0;
}

const adt_bytes_t* apx_nodeInfo_getProvidePortUnpackProgram(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId < self->numProvidePorts) && (self->providePortUnpackPrograms != 0) )
   {
      return self->providePortUnpackPrograms[portId];
   }
   return (const adt_bytes_t*) 0;
}

apx_portId_t apx_nodeInfo_findProvidePortIdFromByteOffset(const apx_nodeInfo_t *self, apx_offset_t offset)
{
   if ( (self != 0) && (offset >=0) && (self->serverBytePortMap != 0))
   {
      return apx_bytePortMap_lookup(self->serverBytePortMap, offset);
   }
   return (apx_portId_t) -1;
}

apx_portId_t apx_nodeInfo_findRequirePortIdFromByteOffset(const apx_nodeInfo_t *self, apx_offset_t offset)
{
   if ( (self != 0) && (offset >=0) && (self->clientBytePortMap != 0))
   {
      return apx_bytePortMap_lookup(self->clientBytePortMap, offset);
   }
   return (apx_portId_t) -1;
}

apx_size_t apx_nodeInfo_calcRequirePortDataLen(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return apx_portDataProps_sumDataSize(self->requirePortDataProps, self->numRequirePorts);
   }
   return (apx_size_t) 0u;
}

apx_size_t apx_nodeInfo_calcProvidePortDataLen(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return apx_portDataProps_sumDataSize(self->providePortDataProps, self->numProvidePorts);
   }
   return (apx_size_t) 0u;
}

apx_size_t apx_nodeInfo_getRequirePortDataLen(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->requirePortDataLen;
   }
   return 0u;
}

apx_size_t apx_nodeInfo_getProvidePortDataLen(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->providePortDataLen;
   }
   return 0u;
}


apx_bytePortMap_t *apx_nodeInfo_getClientBytePortMap(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->clientBytePortMap;
   }
   return (apx_bytePortMap_t*) 0;
}

apx_bytePortMap_t *apx_nodeInfo_getServerBytePortMap(const apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->serverBytePortMap;
   }
   return (apx_bytePortMap_t*) 0;
}

apx_size_t apx_nodeInfo_getRequirePortInitDataSize(const apx_nodeInfo_t *self)
{
   if ( (self != 0) && (self->requirePortInitData != 0))
   {
      return adt_bytes_length(self->requirePortInitData);
   }
   return (apx_size_t) 0;
}

const uint8_t *apx_nodeInfo_getRequirePortInitDataPtr(const apx_nodeInfo_t *self)
{
   if ( (self != 0) && (self->requirePortInitData != 0))
   {
      return adt_bytes_constData(self->requirePortInitData);
   }
   return (const uint8_t*) 0;
}

apx_size_t apx_nodeInfo_getProvidePortInitDataSize(const apx_nodeInfo_t *self)
{
   if ( (self != 0) && (self->providePortInitData != 0))
   {
      return adt_bytes_length(self->providePortInitData);
   }
   return (apx_size_t) 0;
}

const uint8_t *apx_nodeInfo_getProvidePortInitDataPtr(const apx_nodeInfo_t *self)
{
   if ( (self != 0) && (self->providePortInitData != 0))
   {
      return adt_bytes_constData(self->providePortInitData);
   }
   return (const uint8_t*) 0;
}

const char *apx_nodeInfo_getRequirePortSignature(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numRequirePorts) )
   {
      if (self->requirePortSignatures != 0)
      {
         return self->requirePortSignatures[portId];
      }
   }
   return (const char*) 0;
}

const char *apx_nodeInfo_getProvidePortSignature(const apx_nodeInfo_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numProvidePorts) )
   {
      if (self->providePortSignatures != 0)
      {
         return self->providePortSignatures[portId];
      }
   }
   return (const char*) 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_nodeInfo_allocateMemory(apx_nodeInfo_t *self)
{
   if (self->numRequirePorts > 0)
   {
      self->requirePortDataProps = (apx_portDataProps_t*) malloc(sizeof(apx_portDataProps_t)*self->numRequirePorts);
      if (self->requirePortDataProps == 0)
      {
         return APX_MEM_ERROR;
      }
      size_t numBytes = sizeof(adt_bytes_t*) * self->numRequirePorts;
      self->requirePortPackPrograms = (adt_bytes_t**) malloc(numBytes);
      if (self->requirePortPackPrograms == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(self->requirePortPackPrograms, 0, numBytes);

      self->requirePortUnpackPrograms = (adt_bytes_t**) malloc(numBytes);
      if (self->requirePortUnpackPrograms == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(self->requirePortUnpackPrograms, 0, numBytes);
   }
   if (self->numProvidePorts > 0)
   {
      self->providePortDataProps = (apx_portDataProps_t*) malloc(sizeof(apx_portDataProps_t)*self->numProvidePorts);
      if (self->providePortDataProps == 0)
      {
         return APX_MEM_ERROR;
      }
      size_t numBytes = sizeof(adt_bytes_t*) * self->numProvidePorts;
      self->providePortPackPrograms = (adt_bytes_t**) malloc(numBytes);
      if (self->providePortPackPrograms == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(self->providePortPackPrograms, 0, numBytes);

      self->providePortUnpackPrograms = (adt_bytes_t**) malloc(numBytes);
      if (self->providePortUnpackPrograms == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(self->providePortUnpackPrograms, 0, numBytes);
   }
   return APX_NO_ERROR;
}

static void apx_nodeInfo_freeMemory(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      if (self->requirePortDataProps != 0)
      {
         free(self->requirePortDataProps);
      }
      if (self->providePortDataProps != 0)
      {
         free(self->providePortDataProps);
      }
      if (self->requirePortPackPrograms != 0)
      {
         apx_portId_t portId;
         for(portId = 0; portId<self->numRequirePorts; portId++)
         {
            adt_bytes_delete(self->requirePortPackPrograms[portId]);
         }
         free(self->requirePortPackPrograms);
         self->requirePortPackPrograms = 0;
      }
      if (self->providePortPackPrograms != 0)
      {
         apx_portId_t portId;
         for(portId = 0; portId<self->numProvidePorts; portId++)
         {
            adt_bytes_delete(self->providePortPackPrograms[portId]);
         }
         free(self->providePortPackPrograms);
         self->providePortPackPrograms = 0;
      }
      if (self->requirePortUnpackPrograms != 0)
      {
         apx_portId_t portId;
         for(portId = 0; portId<self->numRequirePorts; portId++)
         {
            adt_bytes_delete(self->requirePortUnpackPrograms[portId]);
         }
         free(self->requirePortUnpackPrograms);
         self->requirePortUnpackPrograms = 0;
      }
      if (self->providePortUnpackPrograms != 0)
      {
         apx_portId_t portId;
         for(portId = 0; portId<self->numProvidePorts; portId++)
         {
            adt_bytes_delete(self->providePortUnpackPrograms[portId]);
         }
         free(self->providePortUnpackPrograms);
         self->providePortUnpackPrograms = 0;
      }
      if (self->clientBytePortMap != 0)
      {
         apx_bytePortMap_delete(self->clientBytePortMap);
      }
      if (self->serverBytePortMap != 0)
      {
         apx_bytePortMap_delete(self->serverBytePortMap);
      }
      if (self->requirePortInitData != 0)
      {
         adt_bytes_delete(self->requirePortInitData);
      }
      if (self->providePortInitData != 0)
      {
         adt_bytes_delete(self->providePortInitData);
      }
      if (self->name != 0)
      {
         free(self->name);
      }
      if (self->requirePortSignatures != 0)
      {
         apx_portId_t portId;
         for(portId = 0; portId < ((apx_portId_t)self->numRequirePorts); portId++)
         {
            if (self->requirePortSignatures[portId] != 0)
            {
               free(self->requirePortSignatures[portId]);
            }
         }
         free(self->requirePortSignatures);
         self->requirePortSignatures = 0;
      }
      if (self->providePortSignatures != 0)
      {
         apx_portId_t portId;
         for(portId = 0; portId < ((apx_portId_t)self->numProvidePorts); portId++)
         {
            if (self->providePortSignatures[portId] != 0)
            {
               free(self->providePortSignatures[portId]);
            }
         }
         free(self->providePortSignatures);
         self->providePortSignatures = 0;
      }
   }
}

static void apx_nodeInfo_createRequirePortDataProps(apx_nodeInfo_t *self)
{
   if (self->numRequirePorts > 0)
   {
      apx_portId_t portId;
      apx_size_t offset = 0;
      for(portId=0;portId<self->numRequirePorts;portId++)
      {
         apx_portDataProps_t *props = &self->requirePortDataProps[portId];
         apx_size_t dataSize = 0u;
         uint8_t programFlags = 0u;
         //apx_port_t *port = apx_node_getRequirePort(node, portId);
         const adt_bytes_t *program = apx_nodeInfo_getRequirePortUnpackProgram(self, portId);
         assert(program != 0);
         apx_error_t rc = apx_vm_decodeProgramDataProps(program, &dataSize, &programFlags);
         assert(rc == APX_NO_ERROR);
         assert(dataSize > 0);
         apx_portDataProps_create(props, APX_REQUIRE_PORT, portId, offset, dataSize);
         offset += dataSize;
      }
   }
}

static void apx_nodeInfo_createProvidePortDataProps(apx_nodeInfo_t *self)
{
   if (self->numProvidePorts > 0)
   {
      apx_portId_t portId;
      apx_size_t offset = 0;
      for(portId=0;portId<self->numProvidePorts;portId++)
      {
         apx_portDataProps_t *props = &self->providePortDataProps[portId];
         apx_size_t dataSize = 0u;
         uint8_t programFlags = 0u;
         //apx_port_t *port = apx_node_getRequirePort(node, portId);
         const adt_bytes_t *program = apx_nodeInfo_getProvidePortPackProgram(self, portId);
         assert(program != 0);
         apx_error_t rc = apx_vm_decodeProgramDataProps(program, &dataSize, &programFlags);
         assert(rc == APX_NO_ERROR);
         assert(dataSize > 0u);
         apx_portDataProps_create(props, APX_PROVIDE_PORT, portId, offset, dataSize);
         offset += dataSize;
      }
   }
}

static apx_error_t apx_nodeInfo_initClientBytePortMap(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      if ( (self->requirePortDataProps != 0) && (self->numRequirePorts > 0) )
      {
         apx_error_t err = APX_NO_ERROR;
         self->clientBytePortMap = apx_bytePortMap_new(self->requirePortDataProps, self->numRequirePorts, &err);
         if (self->clientBytePortMap == 0)
         {
            return err;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_nodeInfo_initServerBytePortMap(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      if ( (self->providePortDataProps != 0) && (self->numProvidePorts > 0) )
      {
         apx_error_t err = APX_NO_ERROR;
         self->serverBytePortMap = apx_bytePortMap_new(self->providePortDataProps, self->numProvidePorts, &err);
         if (self->serverBytePortMap == 0)
         {
            return err;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


static apx_error_t apx_nodeInfo_compilePortPrograms(apx_nodeInfo_t *self, apx_compiler_t *compiler, const apx_node_t *node, apx_programType_t *errProgramType, apx_uniquePortId_t *errPortId)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != 0) && (compiler != 0) && (node != 0) && (errProgramType != 0) && (errPortId != 0) )
   {
      apx_portId_t portIndex;
      adt_bytearray_t program;
      adt_bytearray_create(&program, APX_PROGRAM_GROW_SIZE);
      if (self->numRequirePorts > 0)
      {
         if ( (self->requirePortPackPrograms == 0) ||  (self->requirePortUnpackPrograms == 0))
         {
            return APX_NULL_PTR_ERROR;
         }
         for(portIndex=0; portIndex < self->numRequirePorts; portIndex++)
         {
            apx_dataElement_t *dataElement;
            apx_error_t compilationResult;
            apx_port_t *port;

            *errProgramType = APX_PACK_PROGRAM;
            port = apx_node_getRequirePort(node, portIndex);
            assert(port != 0);
            dataElement = apx_port_getDerivedDataElement(port);
            assert(dataElement != 0);
            adt_bytearray_clear(&program);
            apx_compiler_begin_packProgram(compiler, &program);
            compilationResult = apx_compiler_compilePackDataElement(compiler, dataElement);
            if (compilationResult == APX_NO_ERROR)
            {
               adt_bytes_t *tmp;
               apx_compiler_end(compiler);
               tmp = adt_bytearray_bytes(&program);
               if (tmp == 0)
               {
                  retval = APX_MEM_ERROR;
                  break;
               }
               self->requirePortPackPrograms[portIndex] = tmp;
            }
            else
            {
               retval = compilationResult;
               break;
            }
            *errProgramType = APX_UNPACK_PROGRAM;
            adt_bytearray_clear(&program);
            apx_compiler_begin_unpackProgram(compiler, &program);
            compilationResult = apx_compiler_compileUnpackDataElement(compiler, dataElement);
            if (compilationResult == APX_NO_ERROR)
            {
               adt_bytes_t *tmp;
               apx_compiler_end(compiler);
               tmp = adt_bytearray_bytes(&program);
               if (tmp == 0)
               {
                  retval = APX_MEM_ERROR;
                  break;
               }
               self->requirePortUnpackPrograms[portIndex] = tmp;
            }
            else
            {
               retval = compilationResult;
               break;
            }
            (*errPortId)++;
         }
      }

      if ( (retval == APX_NO_ERROR) && (self->numProvidePorts > 0) )
      {
         if ( (self->providePortPackPrograms == 0) || (self->providePortUnpackPrograms == 0) )
         {
            return APX_NULL_PTR_ERROR;
         }
         *errPortId = (apx_uniquePortId_t) APX_PORT_ID_PROVIDE_PORT;
         for(portIndex=0; portIndex < self->numProvidePorts; portIndex++)
         {
            apx_dataElement_t *dataElement;
            apx_error_t compilationResult;
            apx_port_t *port;
            *errProgramType = APX_PACK_PROGRAM;
            port = apx_node_getProvidePort(node, portIndex);
            assert(port != 0);
            dataElement = apx_port_getDerivedDataElement(port);
            assert(dataElement != 0);

            adt_bytearray_clear(&program);
            apx_compiler_begin_packProgram(compiler, &program);
            compilationResult = apx_compiler_compilePackDataElement(compiler, dataElement);
            if (compilationResult == APX_NO_ERROR)
            {
               adt_bytes_t *tmp;
               apx_compiler_end(compiler);
               tmp = adt_bytearray_bytes(&program);
               if (tmp == 0)
               {
                  retval = APX_MEM_ERROR;
                  break;
               }
               self->providePortPackPrograms[portIndex] = tmp;
            }
            else
            {
               retval = compilationResult;
               break;
            }
            *errProgramType = APX_UNPACK_PROGRAM;
            adt_bytearray_clear(&program);
            apx_compiler_begin_unpackProgram(compiler, &program);
            compilationResult = apx_compiler_compileUnpackDataElement(compiler, dataElement);
            if (compilationResult == APX_NO_ERROR)
            {
               adt_bytes_t *tmp;
               apx_compiler_end(compiler);
               tmp = adt_bytearray_bytes(&program);
               if (tmp == 0)
               {
                  retval = APX_MEM_ERROR;
                  break;
               }
               self->providePortUnpackPrograms[portIndex] = tmp;
            }
            else
            {
               retval = compilationResult;
               break;
            }
            (*errPortId)++;
         }
      }
      adt_bytearray_destroy(&program);
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

static apx_error_t apx_nodeInfo_createRequirePortInitData(apx_nodeInfo_t *self, const apx_node_t *node)
{
   apx_size_t dataSize;
   assert(self != 0);
   assert(self->numRequirePorts > 0);
   assert(self->requirePortDataProps != 0);
   assert(self->requirePortPackPrograms != 0);
   assert(node != 0);
   assert(node->isFinalized);
   dataSize = apx_portDataProps_sumDataSize(self->requirePortDataProps, self->numRequirePorts);
   if (dataSize > 0)
   {
      apx_error_t errorCode = APX_NO_ERROR;
      uint8_t *initData = apx_nodeInfo_createInitDataBuf(dataSize, self->requirePortPackPrograms, apx_node_getRequirePortList(node), self->numRequirePorts, &errorCode);
      if (initData == 0)
      {
         return errorCode;
      }
      else
      {
         assert(errorCode == APX_NO_ERROR);
         self->requirePortInitData = adt_bytes_new(initData, (uint32_t) dataSize);
         free(initData);
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_nodeInfo_createProvidePortInitData(apx_nodeInfo_t *self, const apx_node_t *node)
{
   apx_size_t dataSize;
   assert(self != 0);
   assert(self->numProvidePorts > 0);
   assert(self->providePortDataProps != 0);
   assert(self->providePortPackPrograms != 0);
   assert(node != 0);
   assert(node->isFinalized);
   dataSize = apx_portDataProps_sumDataSize(self->providePortDataProps, self->numProvidePorts);
   if (dataSize > 0)
   {
      apx_error_t errorCode = APX_NO_ERROR;
      uint8_t *initData = apx_nodeInfo_createInitDataBuf(dataSize, self->providePortPackPrograms, apx_node_getProvidePortList(node), self->numProvidePorts, &errorCode);
      if (initData == 0)
      {
         return errorCode;
      }
      else
      {
         assert(errorCode == APX_NO_ERROR);
         self->providePortInitData = adt_bytes_new(initData, (uint32_t) dataSize);
         free(initData);
      }
   }
   return APX_NO_ERROR;
}

static uint8_t* apx_nodeInfo_createInitDataBuf(apx_size_t dataSize, adt_bytes_t **packPrograms, const adt_ary_t *ports, apx_portCount_t numPorts, apx_error_t *errorCode)
{
   uint8_t *initData;
   assert(dataSize > 0);
   assert(packPrograms != 0);
   assert(ports != 0);
   assert(numPorts > 0);
   assert(errorCode != 0);
   if (adt_ary_length(ports) != numPorts)
   {
      *errorCode = APX_LENGTH_ERROR;
      return (uint8_t*) 0;
   }
   initData = (uint8_t*) malloc(dataSize);
   if (initData != 0)
   {
      apx_portId_t portId;
      apx_error_t result;
      apx_vm_t vm;
      apx_vm_create(&vm);
      result = apx_vm_setWriteBuffer(&vm, initData, (uint32_t) dataSize);
      if (result == APX_NO_ERROR)
      {
         for(portId = 0; portId < numPorts; portId++)
         {
            dtl_dv_t *properInitValue;
            apx_port_t *port = (apx_port_t*) adt_ary_value(ports, portId);
            assert(port != 0);
            properInitValue = apx_port_getProperInitValue(port);
            result = apx_vm_selectProgram(&vm, packPrograms[portId]);
            if (result == APX_NO_ERROR)
            {
               if (properInitValue != 0)
               {
                  result = apx_vm_packValue(&vm, properInitValue);
               }
               else
               {
                  result = apx_vm_writeNullValue(&vm);
               }
            }
            if (result != APX_NO_ERROR)
            {
               break;
            }
         }
      }
      if (result != APX_NO_ERROR)
      {
         free(initData);
         initData = (uint8_t*) 0;
      }
      *errorCode = result;
      apx_vm_destroy(&vm);
   }
   return initData;
}

static apx_error_t apx_nodeInfo_buildRequirePortSignatures(apx_nodeInfo_t *self, const apx_node_t *node)
{
   if (node != 0)
   {
      apx_portId_t portId;
      size_t allocSize = self->numRequirePorts * sizeof(char*);
      assert(allocSize > 0u);
      self->requirePortSignatures = (char**) malloc(allocSize);
      if (self->requirePortSignatures == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(self->requirePortSignatures, 0, allocSize);
      for(portId = 0; portId < ((apx_portId_t)self->numRequirePorts); portId++)
      {
         const char *portSignature;
         apx_port_t *port = apx_node_getRequirePort(node, portId);
         if (port == 0)
         {
            return APX_NULL_PTR_ERROR;
         }
         portSignature = apx_port_getDerivedPortSignature(port);
         if (portSignature == 0)
         {
            return APX_PORT_SIGNATURE_ERROR;
         }
         else
         {
            self->requirePortSignatures[portId] = STRDUP(portSignature);
            if (self->requirePortSignatures[portId] == 0)
            {
               return APX_MEM_ERROR;
            }
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_nodeInfo_buildProvidePortSignatures(apx_nodeInfo_t *self, const apx_node_t *node)
{
   if (node != 0)
   {
      apx_portId_t portId;
      size_t allocSize = self->numProvidePorts * sizeof(char*);
      assert(allocSize > 0u);
      self->providePortSignatures = (char**) malloc(allocSize);
      if (self->providePortSignatures == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(self->providePortSignatures, 0, allocSize);
      for(portId = 0; portId < ((apx_portId_t)self->numProvidePorts); portId++)
      {
         const char *portSignature;
         apx_port_t *port = apx_node_getProvidePort(node, portId);
         if (port == 0)
         {
            return APX_NULL_PTR_ERROR;
         }
         portSignature = apx_port_getDerivedPortSignature(port);
         if (portSignature == 0)
         {
            return APX_PORT_SIGNATURE_ERROR;
         }
         else
         {
            self->providePortSignatures[portId] = STRDUP(portSignature);
            if (self->providePortSignatures[portId] == 0)
            {
               return APX_MEM_ERROR;
            }
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
