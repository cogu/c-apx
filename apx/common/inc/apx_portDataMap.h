/*****************************************************************************
* \file      apx_portDataMap.h
* \author    Conny Gustafsson
* \date      2018-11-26
* \brief     Description
*
* Copyright (c) 2018 Conny Gustafsson
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
#ifndef APX_PORT_DATA_MAP_H
#define APX_PORT_DATA_MAP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_portDataProps.h"
#include "apx_portDataRef.h"
#include "apx_bytePortMap.h"
#include "apx_nodeData.h"
#include "apx_portTriggerList.h"
#include "apx_compiler.h"
#include "adt_bytes.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_portConnectionTable_tag;

typedef struct apx_portDataMap_tag
{
   apx_portDataProps_t *requirePortDataProps; //strong reference to apx_portDataProps_t, length of array: numRequirePorts
   apx_portDataProps_t *providePortDataProps; //strong reference to apx_portDataProps_t, length of array: numProvidePorts
   apx_portDataRef_t *requirePortDataRef; //strong references to apx_portDataRef_t, length of array: numRequirePorts
   apx_portDataRef_t *providePortDataRef; //strong references to apx_portDataRef_t, length of array: numProvidePorts
   apx_bytePortMap_t *requirePortByteMap; //used in client mode, maps byte offset back to require port ID
   apx_bytePortMap_t *providePortByteMap; //used in server mode, maps byte offset back to provide port ID
   apx_portTriggerList_t *portTriggerList; //used in server mode, strong reference to apx_portTriggerList_t, length of array=numProvidePorts
   adt_bytes_t **requirePortPackPrograms; //Strong reference to adt_bytes_t*,length of array: numRequirePorts
   adt_bytes_t **providePortPackPrograms; //Strong reference to adt_bytes_t*,length of array: numRequirePorts
   int32_t numRequirePorts;
   int32_t numProvidePorts;
}apx_portDataMap_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portDataMap_create(apx_portDataMap_t *self, apx_nodeData_t *nodeData);
void apx_portDataMap_destroy(apx_portDataMap_t *self);
apx_portDataMap_t *apx_portDataMap_new(apx_nodeData_t *nodeData);
void apx_portDataMap_delete(apx_portDataMap_t *self);
apx_portDataProps_t *apx_portDataMap_getRequirePortDataProps(apx_portDataMap_t *self, apx_portId_t portId);
apx_portDataProps_t *apx_portDataMap_getProvidePortDataProps(apx_portDataMap_t *self, apx_portId_t portId);
apx_error_t apx_portDataMap_initPortTriggerList(apx_portDataMap_t *self);
apx_error_t apx_portDataMap_initRequirePortByteMap(apx_portDataMap_t *self, apx_node_t *node);
apx_error_t apx_portDataMap_initProvidePortByteMap(apx_portDataMap_t *self, apx_node_t *node);
apx_portDataRef_t *apx_portDataMap_getRequirePortDataRef(apx_portDataMap_t *self, apx_portId_t portId);
apx_portDataRef_t *apx_portDataMap_getProvidePortDataRef(apx_portDataMap_t *self, apx_portId_t portId);
void apx_portDataMap_updatePortTriggerList(apx_portDataMap_t *self, struct apx_portConnectionTable_tag *portConnectionTable);
apx_portTriggerList_t *apx_portDataMap_getPortTriggerList(apx_portDataMap_t *self);
apx_error_t apx_portDataMap_createPackPrograms(apx_portDataMap_t *self, apx_compiler_t *compiler, apx_node_t *node, apx_uniquePortId_t *errPortId);


#endif //APX_PORT_DATA_MAP_H
