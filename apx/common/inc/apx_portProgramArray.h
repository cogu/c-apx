/*****************************************************************************
* \file      apx_portProgramArray.h
* \author    Conny Gustafsson
* \date      2019-01-02
* \brief     A container for APX port programs
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
#ifndef APX_PORT_PROGRAM_LIST_H
#define APX_PORT_PROGRAM_LIST_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "adt_ary.h"
#include "adt_bytearray.h"
#include "apx_error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portProgramArray_tag
{
   adt_bytearray_t **programs; //array containing strong references to adt_bytearray_t
   int32_t numPorts;
}apx_portProgramArray_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portProgramArray_create(apx_portProgramArray_t *self, int32_t numPorts);
void apx_portProgramArray_destroy(apx_portProgramArray_t *self);
apx_portProgramArray_t *apx_portProgramArray_new(int32_t numPorts);
void apx_portProgramArray_delete(apx_portProgramArray_t *self);

void apx_portProgramArray_set(apx_portProgramArray_t *self, apx_portId_t portId, adt_bytearray_t *program);
adt_bytearray_t* apx_portProgramArray_get(apx_portProgramArray_t *self, apx_portId_t portId);

#endif //APX_PORT_PROGRAM_LIST_H
