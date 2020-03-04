/*****************************************************************************
* \file      apx_portConnectorList.h
* \author    Conny Gustafsson
* \date      2018-12-07
* \brief     Internal lookup table for port subscriptions
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
#ifndef APX_PORT_TRIGGER_LIST_H
#define APX_PORT_TRIGGER_LIST_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "apx_error.h"
#include "apx_portDataRef.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * Keeps a list of port connectors from one p-port to zero or more r-ports.
 */
typedef struct apx_portConnectorList_tag
{
   adt_ary_t requirePortData; //weak references to apx_portRef_t
} apx_portConnectorList_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//dataTriggerTable
void apx_portConnectorList_create(apx_portConnectorList_t *self);
void apx_portConnectorList_destroy(apx_portConnectorList_t *self);
apx_portConnectorList_t* apx_portConnectorList_new(void);
void apx_portConnectorList_delete(apx_portConnectorList_t *self);

apx_error_t apx_portConnectorList_insert(apx_portConnectorList_t *self, apx_portRef_t *portData);
void apx_portConnectorList_remove(apx_portConnectorList_t *self, apx_portRef_t *portData);
void apx_portConnectorList_clear(apx_portConnectorList_t *self);
int32_t apx_portConnectorList_length(apx_portConnectorList_t *self);
apx_portRef_t *apx_portConnectorList_get(apx_portConnectorList_t *self, int32_t index);

#endif //APX_PORT_TRIGGER_LIST_H
