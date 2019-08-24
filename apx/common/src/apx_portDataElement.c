/*****************************************************************************
* \file      apx_portDataElement.c
* \author    Conny Gustafsson
* \date      2018-10-08
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
#include <malloc.h>
#include "apx_error.h"
#include "apx_portDataElement.h"
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
void apx_portDataElement_create(apx_portDataElement_t *self, apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t elemSize)
{
   if (self != 0)
   {
      self->portType = portType;
      self->portId = portId;
      self->offset = offset;
      self->elemSize = elemSize;
      self->totalSize = elemSize;
      self->queLenType = APX_QUE_LEN_NONE;
      self->maxQueLen = 0;
   }
}

apx_portDataElement_t *apx_portDataElement_new(apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t elemSize)
{
   apx_portDataElement_t *self = (apx_portDataElement_t*) malloc(sizeof(apx_portDataElement_t));
   if(self != 0)
   {
      apx_portDataElement_create(self, portType, portId, offset, elemSize);
   }
   return self;
}

void apx_portDataElement_delete(apx_portDataElement_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_portDataElement_vdelete(void *arg)
{
   apx_portDataElement_delete((apx_portDataElement_t*) arg);
}

bool apx_portDataElement_isPlainOldData(apx_portDataElement_t *self)
{
   if ( (self != 0) && (!self->isDynamicArray) && (self->queLenType == APX_QUE_LEN_NONE) )
   {
      return true;
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


