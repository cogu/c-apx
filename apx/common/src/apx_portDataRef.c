/*****************************************************************************
* \file      apx_portDataRef.c
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
#include <string.h>
#include "apx_error.h"
#include "apx_portDataRef.h"
#include "apx_portDataMap.h"
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
void apx_portDataRef_create(apx_portDataRef_t *self, struct apx_nodeData_tag *nodeData, apx_uniquePortId_t portId, apx_portDataAttributes_t *attributes)
{
   if (self != 0)
   {
      self->nodeData = nodeData;
      self->portId = portId;
      self->attributes = attributes;
   }
}

apx_portDataRef_t *apx_portDataRef_new(struct apx_nodeData_tag *nodedata, apx_uniquePortId_t portId, apx_portDataAttributes_t *attributes)
{
   apx_portDataRef_t *self = (apx_portDataRef_t*) malloc(sizeof(apx_portDataRef_t));
   if(self != 0)
   {
      apx_portDataRef_create(self, nodedata, portId, attributes);
   }
   return self;
}

void apx_portDataRef_delete(apx_portDataRef_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_portDataRef_vdelete(void *arg)
{
   apx_portDataRef_delete((apx_portDataRef_t*) arg);
}

bool apx_portDataRef_isProvidePortRef(apx_portDataRef_t *self)
{
   return ( (self != 0) && ( (self->portId & APX_PORT_ID_PROVIDE_PORT) != 0u ) );
}

apx_portId_t apx_portDataRef_getPortId(apx_portDataRef_t *self)
{
   if (self != 0)
   {
      return self->portId & APX_PORT_ID_MASK;
   }
   return -1;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


