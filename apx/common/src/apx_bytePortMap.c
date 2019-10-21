/*****************************************************************************
* \file      apx_bytePortMap.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map
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
#include <assert.h>
#include "apx_bytePortMap.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_bytePortMap_generate(apx_bytePortMap_t *self, adt_ary_t *portList, uint32_t numBytesInFile);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_bytePortMap_create(apx_bytePortMap_t *self, apx_node_t *node, apx_portType_t portType)
{
   apx_error_t retval = APX_INVALID_ARGUMENT_ERROR;
   if ( (self != 0) && (node != 0) )
   {
      retval = APX_NO_ERROR;
      self->mapData = (apx_portId_t*) 0;
      self->mapLen = 0;
      if (portType == APX_REQUIRE_PORT)
      {
         int32_t fileLen = apx_node_calcInPortDataLen(node);
         if (fileLen > 0)
         {
            retval = apx_bytePortMap_generate(self, &node->requirePortList, (uint32_t) fileLen);
         }
         else
         {
            retval = APX_LENGTH_ERROR;
         }
      }
      else if (portType == APX_PROVIDE_PORT)
      {
         int32_t fileLen = apx_node_calcOutPortDataLen(node);
         if (fileLen > 0)
         {
            retval = apx_bytePortMap_generate(self, &node->providePortList, (uint32_t) fileLen);
         }
         else
         {
            retval = APX_LENGTH_ERROR;
         }
      }
      else
      {
         retval = APX_INVALID_ARGUMENT_ERROR;
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

apx_bytePortMap_t *apx_bytePortMap_new(apx_node_t *node, apx_portType_t portType, apx_error_t *errorCode)
{
   apx_bytePortMap_t *self = (apx_bytePortMap_t*) malloc(sizeof(apx_bytePortMap_t));
   if (self != 0)
   {
      apx_error_t result = apx_bytePortMap_create(self, node, portType);
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


apx_portId_t apx_bytePortMap_lookup(apx_bytePortMap_t *self, int32_t offset)
{
   if ( (self != 0) && (offset >= 0) && (offset < self->mapLen) )
   {
      return self->mapData[offset];
   }
   return -1;
}

apx_size_t apx_bytePortMap_length(apx_bytePortMap_t *self)
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
static apx_error_t apx_bytePortMap_generate(apx_bytePortMap_t *self, adt_ary_t *portList, uint32_t numBytesInFile)
{
   int32_t portIndex;
   int32_t numPorts = adt_ary_length(portList);
   apx_portId_t *pNext;
   apx_portId_t *pEnd;
   self->mapData = (apx_portId_t*) malloc(numBytesInFile*sizeof(apx_portId_t));
   if (self->mapData == 0)
   {
      return APX_MEM_ERROR;
   }
   self->mapLen = numBytesInFile;
   pNext = self->mapData;
   pEnd = pNext + self->mapLen;
   for (portIndex=0; portIndex < numPorts; portIndex++)
   {
      int32_t packLen;
      int32_t i;
      apx_port_t *port = (apx_port_t*) adt_ary_value(portList, portIndex);
      packLen = apx_port_getPackLen(port);
      for(i=0;i<packLen;i++)
      {
         pNext[i] = portIndex;
      }
      pNext+=packLen;
      assert(pNext<=pEnd);
   }
   assert(pNext==pEnd);
   return APX_NO_ERROR;
}


