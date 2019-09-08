/*****************************************************************************
* \file      apx_portDataAttributes.h
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
typedef struct apx_portDataAttributes_tag
{
   apx_portId_t portId;
   apx_size_t dataSize; //Size of the data portion in the port data
   apx_size_t totalSize; //Total size of port data, including possible headers
   apx_offset_t offset; //offset in file
   apx_portType_t portType; //Is this a provide or require port?
   apx_dynLenType_t dynLenType; //Is this a dynamically sized array port?
   apx_queLenType_t queLenType; //Is this a queued port?
   apx_size_t maxDynLen; //What is the maximum length of the dynamic array?
   apx_size_t maxQueLen; //What is the maximum number of queued elements?
}apx_portDataAttributes_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portDataAttributes_create(apx_portDataAttributes_t *self, apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t dataSize);
apx_portDataAttributes_t *apx_portDataAttributes_new(apx_portType_t portType, apx_portId_t portIndex, apx_offset_t offset, apx_size_t dataSize);
void apx_portDataAttributes_delete(apx_portDataAttributes_t *self);
void apx_portDataAttributes_vdelete(void *arg);

bool apx_portDataAttributes_isPlainOldData(apx_portDataAttributes_t *self);

#endif //APX_PORT_DATA_ATTRIBUTES_H
