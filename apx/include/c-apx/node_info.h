/*****************************************************************************
* \file      apx_nodeInfo.h
* \author    Conny Gustafsson
* \date      2019-11-25
* \brief     Static information about an APX node.
*            This a post-processed version based on information from the APX-node parse tree.
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
#ifndef APX_NODE_INFO_H
#define APX_NODE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_portDataProps.h"
#include "apx_bytePortMap.h"
#include "adt_bytes.h"
#include "apx_compiler.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration
struct apx_node_tag;

typedef struct apx_nodeInfo_tag
{
   char *name; //Name of the APX node
   apx_portDataProps_t *requirePortDataProps; //strong reference to apx_portDataProps_t, length of array: numRequirePorts
   apx_portDataProps_t *providePortDataProps; //strong reference to apx_portDataProps_t, length of array: numProvidePorts
   apx_bytePortMap_t *clientBytePortMap; //used in client mode, maps byte offset back to require port ID
   apx_bytePortMap_t *serverBytePortMap; //used in server mode, maps byte offset back to provide port ID
   adt_bytes_t **requirePortPackPrograms; //Strong reference to adt_bytes_t*;length of array: numRequirePorts
   adt_bytes_t **providePortPackPrograms; //Strong reference to adt_bytes_t*;length of array: numProvidePorts
   adt_bytes_t **requirePortUnpackPrograms; //Strong reference to adt_bytes_t*;length of array: numRequirePorts
   adt_bytes_t **providePortUnpackPrograms; //Strong reference to adt_bytes_t*;length of array: numProvidePorts
   adt_bytes_t *requirePortInitData; //Calculated init data for requirePorts
   adt_bytes_t *providePortInitData; //Calculated init data for providePorts
   char **requirePortSignatures; //array of derived port signatures strings (used in server mode); length of array: numRequirePorts
   char **providePortSignatures; //array of derived port signatures strings (used in server mode); length of array: numProvidePorts
   apx_portCount_t numRequirePorts;
   apx_portCount_t numProvidePorts;
   apx_size_t requirePortDataLen; //Cached result from apx_nodeInfo_calcRequirePortDataLen
   apx_size_t providePortDataLen; //Cached result from apx_nodeInfo_calcProvidePortDataLen
   apx_mode_t mode; //The mode this nodeInfo was built for
} apx_nodeInfo_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeInfo_create(apx_nodeInfo_t *self);
void apx_nodeInfo_destroy(apx_nodeInfo_t *self);
apx_nodeInfo_t *apx_nodeInfo_new(void);
void apx_nodeInfo_delete(apx_nodeInfo_t *self);

apx_error_t apx_nodeInfo_build(apx_nodeInfo_t *self, const struct apx_node_tag *parseTree, apx_compiler_t *compiler, apx_mode_t mode, apx_programType_t *errProgramType, apx_uniquePortId_t *errPortId);
apx_nodeInfo_t *apx_nodeInfo_make_from_cstr(const char *apx_definition, apx_mode_t mode); //Utility function only meant for unit testing
const char *apx_nodeInfo_getName(const apx_nodeInfo_t *self);
apx_portCount_t apx_nodeInfo_getNumRequirePorts(const apx_nodeInfo_t *self);
apx_portCount_t apx_nodeInfo_getNumProvidePorts(const apx_nodeInfo_t *self);
apx_portDataProps_t *apx_nodeInfo_getRequirePortDataProps(const apx_nodeInfo_t *self, apx_portId_t portId);
apx_portDataProps_t *apx_nodeInfo_getProvidePortDataProps(const apx_nodeInfo_t *self, apx_portId_t portId);
const adt_bytes_t* apx_nodeInfo_getRequirePortPackProgram(const apx_nodeInfo_t *self, apx_portId_t portId);
const adt_bytes_t* apx_nodeInfo_getProvidePortPackProgram(const apx_nodeInfo_t *self, apx_portId_t portId);
const adt_bytes_t* apx_nodeInfo_getRequirePortUnpackProgram(const apx_nodeInfo_t *self, apx_portId_t portId);
const adt_bytes_t* apx_nodeInfo_getProvidePortUnpackProgram(const apx_nodeInfo_t *self, apx_portId_t portId);
apx_portId_t apx_nodeInfo_findProvidePortIdFromByteOffset(const apx_nodeInfo_t *self, apx_offset_t offset);
apx_portId_t apx_nodeInfo_findRequirePortIdFromByteOffset(const apx_nodeInfo_t *self, apx_offset_t offset);
apx_uniquePortId_t apx_nodeInfo_findPortIdByName(const apx_nodeInfo_t *self, const char *name);

apx_size_t apx_nodeInfo_calcRequirePortDataLen(const apx_nodeInfo_t *self); //DEPRECATED; use apx_nodeInfo_getRequirePortDataLen instead
apx_size_t apx_nodeInfo_calcProvidePortDataLen(const apx_nodeInfo_t *self); //DEPRECATED; use apx_nodeInfo_getProvidePortDataLen instead
apx_size_t apx_nodeInfo_getRequirePortDataLen(const apx_nodeInfo_t *self);
apx_size_t apx_nodeInfo_getProvidePortDataLen(const apx_nodeInfo_t *self);

apx_bytePortMap_t *apx_nodeInfo_getClientBytePortMap(const apx_nodeInfo_t *self);
apx_bytePortMap_t *apx_nodeInfo_getServerBytePortMap(const apx_nodeInfo_t *self);
apx_size_t apx_nodeInfo_getRequirePortInitDataSize(const apx_nodeInfo_t *self);
const uint8_t *apx_nodeInfo_getRequirePortInitDataPtr(const apx_nodeInfo_t *self);
apx_size_t apx_nodeInfo_getProvidePortInitDataSize(const apx_nodeInfo_t *self);
const uint8_t *apx_nodeInfo_getProvidePortInitDataPtr(const apx_nodeInfo_t *self);
const char *apx_nodeInfo_getRequirePortSignature(const apx_nodeInfo_t *self, apx_portId_t portId);
const char *apx_nodeInfo_getProvidePortSignature(const apx_nodeInfo_t *self, apx_portId_t portId);
adt_str_t *apx_nodeInfo_getRequirePortName(const apx_nodeInfo_t *self, apx_portId_t portId);
adt_str_t *apx_nodeInfo_getProvidePortName(const apx_nodeInfo_t *self, apx_portId_t portId);


#endif //APX_NODE_INFO_H
