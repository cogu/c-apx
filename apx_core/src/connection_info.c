/*****************************************************************************
* \file      connection_info.c
* \author    Conny Gustafsson
* \date      2021-04-05
* \brief     Connection info used by monitor connections
*
* Copyright (c) 2021 Conny Gustafsson
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
#include "apx/connection_info.h"
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_connectionInfo_create(apx_connectionInfo_t* self, apx_connectionId_t connection_id, adt_str_t* tag)
{
   if (self != NULL)
   {
      self->connection_id = connection_id;
      if (tag != NULL)
      {
         self->tag = adt_str_clone(tag);
      }
   }
}

void apx_connectionInfo_destroy(apx_connectionInfo_t* self)
{
   if (self != NULL)
   {
      adt_str_delete(self->tag); //delete function already has built-in null check
   }
}

apx_connectionInfo_t* apx_connectionInfo_new(apx_connectionId_t connection_id, adt_str_t* tag)
{
   apx_connectionInfo_t* self = (apx_connectionInfo_t*)malloc(sizeof(apx_connectionInfo_t));
   if (self != NULL)
   {
      apx_connectionInfo_create(self, connection_id, tag);
   }
   return self;
}

void apx_connectionInfo_delete(apx_connectionInfo_t* self)
{
   if (self != NULL)
   {
      apx_connectionInfo_destroy(self);
      free(self);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
