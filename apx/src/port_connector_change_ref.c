/*****************************************************************************
* \file      port_connector_change_ref.c
* \author    Conny Gustafsson
* \date      2020-03-03
* \brief     Simple struct containing two pointers
*
* Copyright (c) 2020 Conny Gustafsson
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
#include "apx/port_connector_change_ref.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_portConnectorChangeRef_create(apx_portConnectorChangeRef_t *self, apx_nodeInstance_t *nodeInstance, apx_portConnectorChangeTable_t *connectorChanges)
{
   if (self != 0)
   {
      self->isConnectorChangeTableWeakRef = false; //By default, this data structure takes ownership of the connectorChanges variable
      self->nodeInstance = nodeInstance;
      self->connectorChanges = connectorChanges;
   }
}

void apx_portConnectorChangeRef_destroy(apx_portConnectorChangeRef_t *self)
{
   if (self != 0)
   {
      if ( (!self->isConnectorChangeTableWeakRef) && (self->connectorChanges != 0) )
      {
         apx_portConnectorChangeTable_delete(self->connectorChanges);
      }
   }
}

apx_portConnectorChangeRef_t *apx_portConnectorChangeRef_new(apx_nodeInstance_t *nodeInstance, apx_portConnectorChangeTable_t *connectorChanges)
{
   apx_portConnectorChangeRef_t *self = (apx_portConnectorChangeRef_t*) malloc(sizeof(apx_portConnectorChangeRef_t));
   if (self != 0)
   {
      apx_portConnectorChangeRef_create(self, nodeInstance, connectorChanges);
   }
   return self;
}

void apx_portConnectorChangeRef_delete(apx_portConnectorChangeRef_t *self)
{
   if (self != 0)
   {
      apx_portConnectorChangeRef_destroy(self);
      free(self);
   }
}

void apx_portConnectorChangeRef_vdelete(void *arg)
{
   apx_portConnectorChangeRef_delete( (apx_portConnectorChangeRef_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
