/*****************************************************************************
* \file      apx_portTriggerList.h
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


typedef struct apx_portTriggerList_tag
{
   adt_ary_t requirePortData; //weak references to apx_portRef_t
}apx_portTriggerList_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//dataTriggerTable
void apx_portTriggerList_create(apx_portTriggerList_t *self);
void apx_portTriggerList_destroy(apx_portTriggerList_t *self);
apx_portTriggerList_t* apx_portTriggerList_new(void);
void apx_portTriggerList_delete(apx_portTriggerList_t *self);

apx_error_t apx_portTriggerList_insert(apx_portTriggerList_t *self, apx_portRef_t *portData);
void apx_portTriggerList_remove(apx_portTriggerList_t *self, apx_portRef_t *portData);
int32_t apx_portTriggerList_length(apx_portTriggerList_t *self);
apx_portRef_t *apx_portTriggerList_get(apx_portTriggerList_t *self, int32_t index);

#endif //APX_PORT_TRIGGER_LIST_H
