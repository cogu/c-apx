/*****************************************************************************
* \file      apx_portDataElement.h
* \author    Conny Gustafsson
* \date      2018-11-25
* \brief     This is port _data_ attributes, not to be confused with apx_portAttributes_t
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
#ifndef APX_PORT_DATA_ATTRIBUTES_H
#define APX_PORT_DATA_ATTRIBUTES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portDataElement_tag
{
   apx_portId_t portId;
   apx_size_t elemSize; //Size of the data portion on the port data
   apx_size_t totalSize; //elementSize+queueHeaderSize
   apx_offset_t offset; //offset in file
   apx_portType_t portType; //Is this a provide or require port?
   apx_queLenType_t queLenType; //Is this a queued port?
   bool isDynamicArray; //True if elementSize can vary from element to element
   apx_size_t maxQueLen; //What is the maximum length of the queue?
} apx_portDataElement_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portDataElement_create(apx_portDataElement_t *self, apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t elemSize);
apx_portDataElement_t *apx_portDataElement_new(apx_portType_t portType, apx_portId_t portIndex, apx_offset_t offset, apx_size_t elemSize);
void apx_portDataElement_delete(apx_portDataElement_t *self);
void apx_portDataElement_vdelete(void *arg);

bool apx_portDataElement_isPlainOldData(apx_portDataElement_t *self);

#endif //APX_PORT_DATA_ATTRIBUTES_H
