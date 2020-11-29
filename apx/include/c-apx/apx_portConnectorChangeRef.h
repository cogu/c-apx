/*****************************************************************************
* \file      apx_portConnectorChangeRef.h
* \author    Conny Gustafsson
* \date      2020-03-03
* \brief     Simple struct containing two pointers
*
* Copyright (c) 2020 Conny Gustafsson
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
#ifndef APX_PORT_CONNECTOR_CHANGE_REF_H
#define APX_PORT_CONNECTOR_CHANGE_REF_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_nodeInstance.h"
#include "apx_portConnectorChangeTable.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portConnectorChangeRef_tag
{
   bool isConnectorChangeTableWeakRef;
   apx_nodeInstance_t *nodeInstance;
   apx_portConnectorChangeTable_t *connectorChanges;
} apx_portConnectorChangeRef_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portConnectorChangeRef_create(apx_portConnectorChangeRef_t *self, apx_nodeInstance_t *nodeInstance, apx_portConnectorChangeTable_t *connectorChanges);
void apx_portConnectorChangeRef_destroy(apx_portConnectorChangeRef_t *self);
apx_portConnectorChangeRef_t *apx_portConnectorChangeRef_new(apx_nodeInstance_t *nodeInstance, apx_portConnectorChangeTable_t *connectorChanges);
void apx_portConnectorChangeRef_delete(apx_portConnectorChangeRef_t *self);
void apx_portConnectorChangeRef_vdelete(void *arg);

#endif //APX_PORT_CONNECTOR_CHANGE_REF_H
