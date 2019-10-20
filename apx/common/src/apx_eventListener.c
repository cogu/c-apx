/*****************************************************************************
* \file      apx_eventListener.c
* \author    Conny Gustafsson
* \date      2018-08-05
* \brief     APX event listener
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
#include "apx_eventListener.h"
#include <malloc.h>

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

apx_clientEventListener_t *apx_clientEventListener_clone(apx_clientEventListener_t *other)
{
   if (other != 0)
   {
      apx_clientEventListener_t *self = (apx_clientEventListener_t*) malloc(sizeof(apx_clientEventListener_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_clientEventListener_t*) 0;
}

void apx_clientEventListener_delete(apx_clientEventListener_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_clientEventListener_vdelete(void *arg)
{
   apx_clientEventListener_delete((apx_clientEventListener_t*) arg);
}

apx_serverEventListener_t *apx_serverEventListener_clone(apx_serverEventListener_t *other)
{
   if (other != 0)
   {
      apx_serverEventListener_t *self = (apx_serverEventListener_t*) malloc(sizeof(apx_serverEventListener_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_serverEventListener_t*) 0;
}

void apx_serverEventListener_delete(apx_serverEventListener_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_serverEventListener_vdelete(void *arg)
{
   apx_serverEventListener_delete((apx_serverEventListener_t*) arg);
}

apx_fileManagerEventListener_t *apx_fileManagerEventListener_clone(apx_fileManagerEventListener_t *other)
{
   if (other != 0)
   {
      apx_fileManagerEventListener_t *self = (apx_fileManagerEventListener_t*) malloc(sizeof(apx_fileManagerEventListener_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_fileManagerEventListener_t*) 0;
}

void apx_fileManagerEventListener_delete(apx_fileManagerEventListener_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_fileManagerEventListener_vdelete(void *arg)
{
   apx_fileManagerEventListener_delete((apx_fileManagerEventListener_t *) arg);
}

apx_nodeDataEventListener_t *apx_nodeDataEventListener_clone(apx_nodeDataEventListener_t *other)
{
   if (other != 0)
   {
      apx_nodeDataEventListener_t *self = (apx_nodeDataEventListener_t*) malloc(sizeof(apx_nodeDataEventListener_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_nodeDataEventListener_t*) 0;
}

void apx_nodeDataEventListener_delete(apx_nodeDataEventListener_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_nodeDataEventListener_vdelete(void *arg)
{
   apx_nodeDataEventListener_delete((apx_nodeDataEventListener_t *) arg);
}

apx_fileEventListener_t *apx_fileEventListener_clone(apx_fileEventListener_t *other)
{
   if (other != 0)
   {
      apx_fileEventListener_t *self = (apx_fileEventListener_t*) malloc(sizeof(apx_fileEventListener_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_fileEventListener_t*) 0;
}

void apx_fileEventListener_delete(apx_fileEventListener_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_fileEventListener_vdelete(void *arg)
{
   apx_fileEventListener_delete((apx_fileEventListener_t *) arg);
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


