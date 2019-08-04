/*****************************************************************************
* \file      apx_portDataAttributes.c
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
#include "apx_portDataAttributes.h"
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
void apx_portDataAttributes_create(apx_portDataAttributes_t *self, apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t dataSize)
{
   if (self != 0)
   {
      self->portType = portType;
      self->portId = portId;
      self->offset = offset;
      self->dataSize = dataSize;
      self->totalSize = dataSize;
      self->dynLenType = APX_DYN_LEN_NONE;
      self->queLenType = APX_QUE_LEN_NONE;
      self->maxDynLen = 0;
      self->maxQueLen = 0;
   }
}

apx_portDataAttributes_t *apx_portDataAttributes_new(apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t dataSize)
{
   apx_portDataAttributes_t *self = (apx_portDataAttributes_t*) malloc(sizeof(apx_portDataAttributes_t));
   if(self != 0)
   {
      apx_portDataAttributes_create(self, portType, portId, offset, dataSize);
   }
   return self;
}

void apx_portDataAttributes_delete(apx_portDataAttributes_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_portDataAttributes_vdelete(void *arg)
{
   apx_portDataAttributes_delete((apx_portDataAttributes_t*) arg);
}

bool apx_portDataAttributes_isPlainOldData(apx_portDataAttributes_t *self)
{
   if ( (self != 0) && (self->dynLenType == APX_DYN_LEN_NONE) && (self->queLenType == APX_QUE_LEN_NONE) )
   {
      return true;
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


