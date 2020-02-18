/*****************************************************************************
* \file      apx_portConnectionEntry.h
* \author    Conny Gustafsson
* \date      2019-01-23
* \brief     APX port connection information (for one port)
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
#ifndef APX_PORT_CONNECTION_ENTRY_H
#define APX_PORT_CONNECTION_ENTRY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "apx_types.h"
#include "apx_portDataRef.h"
#include "apx_error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portConnectionEntry_tag
{
   int32_t count; //initial value is 0. When in negative range it holds port disconnect info. When in positive range it holds port connect info.
   union portref_union_tag {
      apx_portRef_t* portRef; //Used when -1 <= count <= 1
      adt_ary_t *array; //Used when count<-1 or when count > 1
   } data;
   //All references to apx_portRef_t are weak references
} apx_portConnectionEntry_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portConnectionEntry_create(apx_portConnectionEntry_t *self);
void apx_portConnectionEntry_destroy(apx_portConnectionEntry_t *self);
apx_portConnectionEntry_t *apx_portConnectionEntry_new(void);
void apx_portConnectionEntry_delete(apx_portConnectionEntry_t *self);
apx_error_t apx_portConnectionEntry_addConnection(apx_portConnectionEntry_t *self, apx_portRef_t *portDataRef);
apx_error_t apx_portConnectionEntry_removeConnection(apx_portConnectionEntry_t *self, apx_portRef_t *portDataRef);
apx_portRef_t *apx_portConnectionEntry_get(apx_portConnectionEntry_t *self, int32_t index);
int32_t apx_portConnectionEntry_count(apx_portConnectionEntry_t *self);

#endif //APX_PORT_CONNECTION_ENTRY_H
