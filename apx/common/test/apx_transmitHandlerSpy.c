/*****************************************************************************
* \file      apx_transmitHandlerSpy.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_transmitHandlerSpy.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_transmitHandlerSpy_create(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      self->buf = 0;
      self->transmitted = adt_ary_new(adt_bytearray_vdelete);
   }
}

void apx_transmitHandlerSpy_destroy(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      if (self->buf != 0)
      {
         adt_bytearray_delete(self->buf);
      }
      adt_ary_delete(self->transmitted);
   }
}

int32_t apx_transmitHandlerSpy_length(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      return adt_ary_length(self->transmitted);
   }
   return -1;
}

adt_bytearray_t *apx_transmitHandlerSpy_next(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      return (adt_bytearray_t*) adt_ary_shift(self->transmitted);
   }
   return (adt_bytearray_t*) 0;
}

uint8_t* apx_transmitHandlerSpy_getSendBuffer(void *arg, int32_t msgLen)
{
   apx_transmitHandlerSpy_t* self = (apx_transmitHandlerSpy_t*) arg;
   if (self != 0)
   {
      self->buf = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
      adt_bytearray_resize(self->buf, msgLen);
      return (adt_bytearray_data(self->buf));
   }
   return 0;
}

int32_t apx_transmitHandlerSpy_send(void *arg, int32_t offset, int32_t msgLen)
{
   apx_transmitHandlerSpy_t* self = (apx_transmitHandlerSpy_t*) arg;
   if ( (self != 0) && (adt_bytearray_length(self->buf) >= (uint32_t) msgLen) )
   {
      adt_ary_push(self->transmitted, self->buf);
      self->buf = 0;
      return msgLen;
   }
   return -1;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


