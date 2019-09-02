/*****************************************************************************
* \file      apx_portDataRef.h
* \author    Conny Gustafsson
* \date      2018-10-08
* \brief     Collects all useful information about a specific port into a single container
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
#ifndef APX_PORT_DATA_H
#define APX_PORT_DATA_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_event.h"
#include "apx_portDataProps.h"
#include <stdbool.h>


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_nodeData_tag;
struct apx_file2_tag;

typedef struct apx_portDataRef_tag
{
   struct apx_nodeData_tag *nodeData; //weak reference to parent nodeData
   apx_uniquePortId_t portId; //This is a provide-port ID if APX_PORT_ID_PROVIDE_PORT is set, otherwise it's a require-port ID.
   apx_portDataProps_t *portDataProps; //weak reference to port data properties
}apx_portDataRef_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portDataRef_create(apx_portDataRef_t *self, struct apx_nodeData_tag *nodeData, apx_uniquePortId_t portId, apx_portDataProps_t *portDataProps);
apx_portDataRef_t *apx_portDataRef_new(struct apx_nodeData_tag *nodedata, apx_uniquePortId_t portId, apx_portDataProps_t *portDataProps);
void apx_portDataRef_delete(apx_portDataRef_t *self);
void apx_portDataRef_vdelete(void *arg);
bool apx_portDataRef_isProvidePortRef(apx_portDataRef_t *self);
apx_portId_t apx_portDataRef_getPortId(apx_portDataRef_t *self);

#endif //APX_PORT_DATA_H
