/*****************************************************************************
* \file      apx_portRef.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Collects all useful information about a specific port into a single container
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
#include "apx_nodeInstance.h"
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
void apx_portRef_create(apx_portRef_t *self, struct apx_nodeInstance_tag *nodeInstance, apx_uniquePortId_t portId, const apx_portDataProps_t *portDataProps)
{
   if (self != 0)
   {
      self->nodeInstance = nodeInstance;
      self->portId = portId;
      self->portDataProps = portDataProps;
   }
}

apx_portRef_t *apx_portRef_new(struct apx_nodeInstance_tag *nodeInstance, apx_uniquePortId_t portId, const apx_portDataProps_t *portDataProps)
{
   apx_portRef_t *self = (apx_portRef_t*) malloc(sizeof(apx_portRef_t));
   if(self != 0)
   {
      apx_portRef_create(self, nodeInstance, portId, portDataProps);
   }
   return self;
}

void apx_portRef_delete(apx_portRef_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_portRef_vdelete(void *arg)
{
   apx_portRef_delete((apx_portRef_t*) arg);
}

bool apx_portRef_isProvidePort(apx_portRef_t *self)
{
   return ( (self != 0) && ( (self->portId & APX_PORT_ID_PROVIDE_PORT) != 0u ) );
}

apx_portId_t apx_portRef_getPortId(apx_portRef_t *self)
{
   if (self != 0)
   {
      return self->portId & APX_PORT_ID_MASK;
   }
   return -1;
}

const apx_portDataProps_t *apx_portRef_getPortDataProps(apx_portRef_t *self)
{
   if (self != 0)
   {
      return self->portDataProps;
   }
   return (const apx_portDataProps_t*) 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


