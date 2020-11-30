/*****************************************************************************
* \file      byte_port_map.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map
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
#include <assert.h>
#include "apx/byte_port_map.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_bytePortMap_build(apx_bytePortMap_t *self, const apx_portDataProps_t *props, apx_portCount_t numPorts, apx_size_t mapLen);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_bytePortMap_create(apx_bytePortMap_t *self, const apx_portDataProps_t *props, apx_portCount_t numPorts)
{
   apx_error_t retval = APX_INVALID_ARGUMENT_ERROR;
   if ( (self != 0) && (props != 0) && (numPorts > 0))
   {
      retval = APX_NO_ERROR;
      self->mapData = (apx_portId_t*) 0;
      self->mapLen = 0;
      apx_size_t mapLen = apx_portDataProps_sumDataSize(props, numPorts);
      if (mapLen > 0)
      {
         retval = apx_bytePortMap_build(self, props, numPorts, mapLen);
      }
      else
      {
         retval = APX_LENGTH_ERROR;
      }
   }
   return retval;
}

void apx_bytePortMap_destroy(apx_bytePortMap_t *self)
{
   if ( (self != 0) && (self->mapData != 0) )
   {
      free(self->mapData);
   }
}

apx_bytePortMap_t *apx_bytePortMap_new(const apx_portDataProps_t *props, apx_portCount_t numPorts, apx_error_t *errorCode)
{
   apx_bytePortMap_t *self = (apx_bytePortMap_t*) malloc(sizeof(apx_bytePortMap_t));
   if (self != 0)
   {
      apx_error_t result = apx_bytePortMap_create(self, props, numPorts);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = (apx_bytePortMap_t*) 0;
      }
      if (errorCode != 0)
      {
         *errorCode = result;
      }
   }
   return self;
}

void apx_bytePortMap_delete(apx_bytePortMap_t *self)
{
   if (self != 0)
   {
      apx_bytePortMap_destroy(self);
      free(self);
   }
}


apx_portId_t apx_bytePortMap_lookup(const apx_bytePortMap_t *self, int32_t offset)
{
   if ( (self != 0) && (offset >= 0) && (offset < self->mapLen) )
   {
      return self->mapData[offset];
   }
   return -1;
}

apx_size_t apx_bytePortMap_length(const apx_bytePortMap_t *self)
{
   if (self != 0)
   {
      return self->mapLen;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_bytePortMap_build(apx_bytePortMap_t *self, const apx_portDataProps_t *propsArray, apx_portCount_t numPorts, apx_size_t mapLen)
{
   if ( (self != 0) && (propsArray != 0) && (numPorts > 0) && (mapLen > 0u) )
   {
      apx_portId_t portId;
      apx_portId_t *pNext;
      apx_portId_t *pEnd;
      self->mapData = (apx_portId_t*) malloc(mapLen*sizeof(apx_portId_t));
      if (self->mapData == 0)
      {
         return APX_MEM_ERROR;
      }
      self->mapLen = mapLen;
      pNext = self->mapData;
      pEnd = pNext + self->mapLen;
      for (portId=0; portId < numPorts; portId++)
      {
         apx_size_t i;
         apx_size_t packLen = propsArray[portId].dataSize;
         for(i=0u; i < packLen; i++)
         {
            pNext[i] = portId;
         }
         pNext+=packLen;
         assert(pNext<=pEnd);
      }
      assert(pNext==pEnd);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
