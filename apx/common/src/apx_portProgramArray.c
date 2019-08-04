/*****************************************************************************
* \file      apx_portProgramArray.c
* \author    Conny Gustafsson
* \date      2019-01-02
* \brief     Description
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include "apx_error.h"
#include "apx_portProgramArray.h"
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
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portProgramArray_create(apx_portProgramArray_t *self, int32_t numPorts)
{
   if ( (self != 0) && (numPorts > 0) )
   {
      uint32_t dataSize = (uint32_t) (numPorts*sizeof(adt_bytearray_t*));
      self->numPorts = numPorts;
      self->programs = (adt_bytearray_t**) malloc(dataSize);
      if (self->programs != 0)
      {
         memset(self->programs, 0, dataSize);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portProgramArray_destroy(apx_portProgramArray_t *self)
{
   if ( (self != 0) && (self->programs != 0) && (self->numPorts > 0))
   {
      int32_t i;
      for(i=0;i<self->numPorts;i++)
      {
         if (self->programs[i] != 0)
         {
            adt_bytearray_delete(self->programs[i]);
         }
      }
      free(self->programs);
   }
}

apx_portProgramArray_t *apx_portProgramArray_new(int32_t numPorts)
{
   apx_portProgramArray_t *self = (apx_portProgramArray_t*) malloc(sizeof(apx_portProgramArray_t));
   if (self != 0)
   {
      apx_portProgramArray_create(self, numPorts);
   }
   return self;
}

void apx_portProgramArray_delete(apx_portProgramArray_t *self)
{
   if (self != 0)
   {
      apx_portProgramArray_destroy(self);
      free(self);
   }
}

void apx_portProgramArray_set(apx_portProgramArray_t *self, apx_portId_t portId, adt_bytearray_t *program)
{
   if ( (self != 0) && (portId < self->numPorts))
   {
      self->programs[portId] = program;
   }
}

adt_bytearray_t* apx_portProgramArray_get(apx_portProgramArray_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId < self->numPorts))
   {
      return self->programs[portId];
   }
   return (adt_bytearray_t*) 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


