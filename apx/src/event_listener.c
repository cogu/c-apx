/*****************************************************************************
* \file      event_listener.c
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Event listener API
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
#include "apx/event_listener.h"
#include <malloc.h>
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


apx_connectionEventListener_t *apx_connectionEventListener_clone(apx_connectionEventListener_t *other)
{
   if (other != 0)
   {
      apx_connectionEventListener_t *self = (apx_connectionEventListener_t*) malloc(sizeof(apx_connectionEventListener_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_connectionEventListener_t*) 0;
}
void apx_connectionEventListener_delete(apx_connectionEventListener_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_connectionEventListener_vdelete(void *arg)
{
   apx_connectionEventListener_delete((apx_connectionEventListener_t*) arg);
}

apx_fileEventListener2_t *apx_fileEventListener_clone(apx_fileEventListener2_t *other)
{
   if (other != 0)
   {
      apx_fileEventListener2_t *self = (apx_fileEventListener2_t*) malloc(sizeof(apx_fileEventListener2_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_fileEventListener2_t*) 0;
}

void apx_fileEventListener_delete(apx_fileEventListener2_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_fileEventListener_vdelete(void *arg)
{
   apx_fileEventListener_delete((apx_fileEventListener2_t *) arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


