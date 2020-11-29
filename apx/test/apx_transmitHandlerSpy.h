/*****************************************************************************
* \file      transmitHandlerSpy.h
* \author    Conny Gustafsson
* \date      2018-08-19
* \brief     Description
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
#ifndef TRANSMIT_HANDLER_SPY_H
#define TRANSMIT_HANDLER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "adt_ary.h"
#include "apx_transmitHandler.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_transmitHandlerSpy_tag
{
   adt_bytearray_t *buf;
   adt_ary_t *transmitted; //strong references to adt_bytearray_t
}apx_transmitHandlerSpy_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_transmitHandlerSpy_create(apx_transmitHandlerSpy_t *self);
void apx_transmitHandlerSpy_destroy(apx_transmitHandlerSpy_t *self);

int32_t apx_transmitHandlerSpy_length(apx_transmitHandlerSpy_t *self);
adt_bytearray_t *apx_transmitHandlerSpy_next(apx_transmitHandlerSpy_t *self);

uint8_t* apx_transmitHandlerSpy_getSendBuffer(void *arg, int32_t msgLen);
int32_t apx_transmitHandlerSpy_send(void *arg, int32_t offset, int32_t msgLen);

#endif //TRANSMIT_HANDLER_SPY_H
