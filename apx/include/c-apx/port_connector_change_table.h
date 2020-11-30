/*****************************************************************************
* \file      port_connector_change_table.h
* \author    Conny Gustafsson
* \date      2019-01-31
* \brief     A list of apx_portConnectionChangeEntry_t. 
*            Used to track changes in port connectors on one side of a node (Require or Provide)
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
#ifndef APX_PORT_CONNECTION_CHANGE_TABLE_H
#define APX_PORT_CONNECTION_CHANGE_TABLE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/port_connector_change_table.h"
#include "apx/port_connector_change_entry.h"
#include "adt_ary.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portConnectorChangeTable_tag
{
   apx_portConnectorChangeEntry_t *entries; //array of apx_portConnectionChangeEntry_t (created using single malloc)
   int32_t numPorts; //This must match nodeInfo->numRequirePorts when this is used for requirePorts or nodeInfo->numProvidePorts when used for providePorts
} apx_portConnectorChangeTable_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portConnectorChangeTable_create(apx_portConnectorChangeTable_t *self, int32_t numPorts);
void apx_portConnectorChangeTable_destroy(apx_portConnectorChangeTable_t *self);
apx_portConnectorChangeTable_t *apx_portConnectorChangeTable_new(int32_t numPorts);
void apx_portConnectorChangeTable_delete(apx_portConnectorChangeTable_t *self);

apx_error_t apx_portConnectorChangeTable_connect(apx_portConnectorChangeTable_t *self, apx_portRef_t *localRef, apx_portRef_t *remoteRef);
apx_error_t apx_portConnectorChangeTable_disconnect(apx_portConnectorChangeTable_t *self, apx_portRef_t *localRef, apx_portRef_t *remoteRef);
apx_portConnectorChangeEntry_t *apx_portConnectorChangeTable_getEntry(apx_portConnectorChangeTable_t *self, apx_portId_t portId);
apx_portRef_t *apx_portConnectorChangeTable_getRef(apx_portConnectorChangeTable_t *self, apx_portId_t portId, int32_t index);
int32_t apx_portConnectorChangeTable_count(apx_portConnectorChangeTable_t *self, apx_portId_t portId);


#endif //APX_PORT_CONNECTION_CHANGE_TABLE_H
