/*****************************************************************************
* \file      port_data_props.c
* \author    Conny Gustafsson
* \date      2018-11-25
* \brief     Port data properties, a combination of apx_dataElement_t and apx_portAttributes_t
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#include "apx/error.h"
#include "apx/port_data_props.h"
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
void apx_portDataProps_create(apx_portDataProps_t *self, apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t dataSize)
{
   if (self != 0)
   {
      self->portType = portType;
      self->portId = portId;
      self->offset = offset;
      self->dataSize = dataSize;
      self->queLenType = APX_QUE_LEN_NONE;
      self->isDynamicArray = false;
      self->maxQueLen = 0;
   }
}

apx_portDataProps_t *apx_portDataProps_new(apx_portType_t portType, apx_portId_t portId, apx_offset_t offset, apx_size_t elemSize)
{
   apx_portDataProps_t *self = (apx_portDataProps_t*) malloc(sizeof(apx_portDataProps_t));
   if(self != 0)
   {
      apx_portDataProps_create(self, portType, portId, offset, elemSize);
   }
   return self;
}

void apx_portDataProps_delete(apx_portDataProps_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_portDataProps_vdelete(void *arg)
{
   apx_portDataProps_delete((apx_portDataProps_t*) arg);
}

bool apx_portDataProps_isPlainOldData(const apx_portDataProps_t *self)
{
   if ( (self != 0) && (!self->isDynamicArray) && (self->queLenType == APX_QUE_LEN_NONE) )
   {
      return true;
   }
   return false;
}

apx_size_t apx_portDataProps_sumDataSize(const apx_portDataProps_t *propsArray, apx_portCount_t numPorts)
{
   apx_size_t sum = 0u;
   if ( (propsArray != 0) && (numPorts > 0) )
   {
      apx_portId_t portId;
      for (portId = 0; portId < numPorts; portId++)
      {
         sum+=propsArray[portId].dataSize;
      }
   }
   return sum;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


