/*****************************************************************************
* \file      apx_portConnectionTable.h
* \author    Conny Gustafsson
* \date      2018-01-31
* \brief     Description
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
#ifndef APX_PORT_CONNECTION_TABLE_H
#define APX_PORT_CONNECTION_TABLE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_error.h"
#include "apx_portConnectionTable.h"
#include "apx_portConnectionEntry.h"
#include "adt_ary.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portConnectionTable_tag
{
   apx_portConnectionEntry_t *connections; //strong references to apx_portConnectionEntry_t
   int32_t numPorts;
} apx_portConnectionTable_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portConnectionTable_create(apx_portConnectionTable_t *self, int32_t numPorts);
void apx_portConnectionTable_destroy(apx_portConnectionTable_t *self);
apx_portConnectionTable_t *apx_portConnectionTable_new(int32_t numPorts);
void apx_portConnectionTable_delete(apx_portConnectionTable_t *self);

apx_error_t apx_portConnectionTable_connect(apx_portConnectionTable_t *self, apx_portDataRef_t *localRef, apx_portDataRef_t *remoteRef);
apx_error_t apx_portConnectionTable_disconnect(apx_portConnectionTable_t *self, apx_portDataRef_t *localRef, apx_portDataRef_t *remoteRef);
/*
apx_error_t apx_portConnectionTable_addConnection(apx_portConnectionTable_t *self, apx_portId_t portId, apx_portDataRef_t *portDataRef);
apx_error_t apx_portConnectionTable_removeConnection(apx_portConnectionTable_t *self, apx_portId_t portId, apx_portDataRef_t *portDataRef);
*/
apx_portConnectionEntry_t *apx_portConnectionTable_getEntry(apx_portConnectionTable_t *self, apx_portId_t portId);
apx_portDataRef_t *apx_portConnectionTable_getRef(apx_portConnectionTable_t *self, apx_portId_t portId, int32_t index);
int32_t apx_portConnectionTable_count(apx_portConnectionTable_t *self, apx_portId_t portId);


#endif //APX_PORT_CONNECTION_TABLE_H
