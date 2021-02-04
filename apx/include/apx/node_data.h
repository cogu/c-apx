/*****************************************************************************
* \file      node_data.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Container for dynamic data of an APX node
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#include "apx/types.h"
#include "apx/error.h"
#include "apx/remotefile.h"
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
struct apx_portInstance_tag;

typedef struct apx_nodeDataBuffers_tag
{
   uint8_t* definition_data;
   uint8_t* provide_port_data;
   uint8_t* require_port_data;
   apx_size_t definition_data_size;
   apx_size_t provide_port_data_size;
   apx_size_t require_port_data_size;
   apx_size_t num_provide_ports;
   apx_size_t num_require_ports;
   apx_connectionCount_t* require_port_connection_count;
   apx_connectionCount_t* provide_port_connection_count;
   uint8_t checksum_data[RMF_SHA256_SIZE];
   rmf_digestType_t checksum_type;
} apx_nodeDataBuffers_t;

typedef struct apx_nodeData_tag
{
   uint8_t* definition_data;
   uint8_t* require_port_data;
   uint8_t* provide_port_data;
   apx_size_t definition_data_size;
   apx_size_t require_port_data_size;
   apx_size_t provide_port_data_size;
   apx_size_t num_require_ports;
   apx_size_t num_provide_ports;
   apx_connectionCount_t* require_port_connection_count; //array-length: num_require_ports
   apx_connectionCount_t* provide_port_connection_count; //array-length: num_provide_ports
   uint8_t checksum_data[RMF_SHA256_SIZE];
   rmf_digestType_t checksum_type;
   struct apx_nodeInstance_tag* parent;
   MUTEX_T lock;
   bool is_weak_ref; //true if memory is managed outside this object
} apx_nodeData_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

////////////////// Constructor/Destructor //////////////////
apx_error_t apx_nodeData_create(apx_nodeData_t *self, apx_nodeDataBuffers_t *buffers);
void apx_nodeData_destroy(apx_nodeData_t *self);
apx_nodeData_t *apx_nodeData_new(void);
void apx_nodeData_delete(apx_nodeData_t *self);
void apx_nodeData_vdelete(void *arg);

////////////////// Data API //////////////////
apx_size_t apx_nodeData_definition_data_size(apx_nodeData_t const* self);
apx_size_t apx_nodeData_provide_port_data_size(apx_nodeData_t const* self);
apx_size_t apx_nodeData_require_port_data_size(apx_nodeData_t const* self);
apx_size_t apx_nodeData_num_provide_ports(apx_nodeData_t const* self);
apx_size_t apx_nodeData_num_require_ports(apx_nodeData_t const* self);
apx_error_t apx_nodeData_create_definition_data(apx_nodeData_t* self, uint8_t const* init_data, apx_size_t data_size);
apx_error_t apx_nodeData_create_provide_port_data(apx_nodeData_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size);
apx_error_t apx_nodeData_create_require_port_data(apx_nodeData_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size);
apx_error_t apx_nodeData_write_definition_data(apx_nodeData_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size);
apx_error_t apx_nodeData_write_provide_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size);
apx_error_t apx_nodeData_read_provide_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size);
apx_error_t apx_nodeData_write_require_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size);
apx_error_t apx_nodeData_read_require_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size);
uint8_t const* apx_nodeData_get_definition_data(apx_nodeData_t const* self);
/*uint8_t const* apx_nodeData_get_provide_port_data(apx_nodeData_t const* self);
uint8_t const* apx_nodeData_get_require_port_data(apx_nodeData_t const* self);*/
uint8_t* apx_nodeData_take_definition_data_snapshot(apx_nodeData_t* self);
uint8_t* apx_nodeData_take_provide_port_data_snapshot(apx_nodeData_t* self);
uint8_t* apx_nodeData_take_require_port_data_snapshot(apx_nodeData_t* self);
void apx_nodeData_set_checksum_data(apx_nodeData_t* self, rmf_digestType_t checksum_type, uint8_t const* checksum_data);
rmf_digestType_t apx_nodeData_get_checksum_type(apx_nodeData_t const* self);
const uint8_t* apx_nodeData_get_checksum_data(apx_nodeData_t const* self);


/*
////////////////// Port Connection Count API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_nodeData_createRequirePortConnectionCountBuffer(apx_nodeData_t* self, apx_portCount_t numRequirePorts);
apx_error_t apx_nodeData_createProvidePortConnectionCountBuffer(apx_nodeData_t* self, apx_portCount_t numProvidePorts);
#endif
apx_connectionCount_t apx_nodeData_getRequirePortConnectionCount(apx_nodeData_t* self, apx_portId_t portId);
apx_connectionCount_t apx_nodeData_getProvidePortConnectionCount(apx_nodeData_t* self, apx_portId_t portId);
void apx_nodeData_incRequirePortConnectionCount(apx_nodeData_t* self, apx_portId_t portId);
void apx_nodeData_incProvidePortConnectionCount(apx_nodeData_t* self, apx_portId_t portId);
void apx_nodeData_decRequirePortConnectionCount(apx_nodeData_t* self, apx_portId_t portId);
void apx_nodeData_decProvidePortConnectionCount(apx_nodeData_t* self, apx_portId_t portId);
uint32_t apx_nodeData_getPortConnectionsTotal(apx_nodeData_t* self);
*/
/*
////////////////// Data Buffer API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_nodeData_createDefinitionBuffer(apx_nodeData_t *self, apx_size_t bufferLen);
#endif
void apx_nodeData_lockDefinitionData(apx_nodeData_t *self);
void apx_nodeData_unlockDefinitionData(apx_nodeData_t *self);
const uint8_t *apx_nodeData_getDefinitionDataBuf(apx_nodeData_t *self);
apx_size_t apx_nodeData_getDefinitionDataLen(apx_nodeData_t *self);
apx_error_t apx_nodeData_writeDefinitionData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readDefinitionData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);


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


////////////////// Utility Functions //////////////////
const char *apx_nodeData_getName(apx_nodeData_t *self);
bool apx_nodeData_isComplete(apx_nodeData_t *self);
uint32_t apx_nodeData_getConnectionId(apx_nodeData_t *self);
*/

#endif //APX_NODE_DATA_H
