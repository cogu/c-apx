/*****************************************************************************
* \file:    apx_serverEventRecorder.h
* \author:  Conny Gustafsson
* \date:    2018-05-01
* \brief:   Listens to internal events from apx_nodeManager/apx_router and transforms them
*           into apx events ready to be send to client
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
#include <string.h>
#include <stdlib.h>
#include "apx_serverEventRecorder.h"
#include "apx_fileManager.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_serverEventRecorder_create(apx_serverEventRecorder_t *self, struct apx_fileManager_tag *parent)
{
   if ( (self != 0) && (self->parent != 0) )
   {
      self->parent = parent;
   }
}

void apx_serverEventRecorder_destroy(apx_serverEventRecorder_t *self)
{
   if (self != 0)
   {
   }
}

apx_serverEventRecorder_t *apx_serverEventRecorder_new(struct apx_fileManager_tag *parent)
{
   apx_serverEventRecorder_t *self = (apx_serverEventRecorder_t*) malloc(sizeof(apx_serverEventRecorder_t));
   if(self != 0)
   {
      apx_serverEventRecorder_create(self, parent);
   }
   return self;
}

void apx_serverEventRecorder_delete(apx_serverEventRecorder_t *self)
{
   if(self != 0)
   {
      apx_serverEventRecorder_destroy(self);
      free(self);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


