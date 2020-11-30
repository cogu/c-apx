/*****************************************************************************
* \file      port_data_ref.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Collects all useful information about a specific port into a single container
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
#ifndef APX_PORT_DATA_REF_H
#define APX_PORT_DATA_REF_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/event.h"
#include "apx/port_data_props.h"
#include <stdbool.h>


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_nodeInstance_tag;

typedef struct apx_portRef_tag
{
   struct apx_nodeInstance_tag *nodeInstance; //weak reference to parent nodeInstance
   const apx_portDataProps_t *portDataProps; //weak reference to port data properties
   apx_uniquePortId_t portId; //This is a provide-port ID if APX_PORT_ID_PROVIDE_PORT is set, otherwise it's a require-port ID.
} apx_portRef_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portRef_create(apx_portRef_t *self, struct apx_nodeInstance_tag *nodeInstance, apx_uniquePortId_t portId, const apx_portDataProps_t *portDataProps);
apx_portRef_t *apx_portRef_new(struct apx_nodeInstance_tag *nodeInstance, apx_uniquePortId_t portId, const apx_portDataProps_t *portDataProps);
void apx_portRef_delete(apx_portRef_t *self);
void apx_portRef_vdelete(void *arg);
bool apx_portRef_isProvidePort(apx_portRef_t *self);
apx_portId_t apx_portRef_getPortId(apx_portRef_t *self);
const apx_portDataProps_t *apx_portRef_getPortDataProps(apx_portRef_t *self);

#endif //APX_PORT_DATA_REF_H
