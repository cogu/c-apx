/*****************************************************************************
* \file      apx_eventListener2.c
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     New event listener
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
#include "apx_eventListener2.h"
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

apx_clientEventListener2_t *apx_clientEventListener2_clone(apx_clientEventListener2_t *other)
{
   if (other != 0)
   {
      apx_clientEventListener2_t *self = (apx_clientEventListener2_t*) malloc(sizeof(apx_clientEventListener2_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_clientEventListener2_t*) 0;
}

void apx_clientEventListener2_delete(apx_clientEventListener2_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}
void apx_clientEventListener2_vdelete(void *arg)
{
   apx_clientEventListener2_delete((apx_clientEventListener2_t*) arg);
}

apx_serverEventListener2_t *apx_serverEventListener2_clone(apx_serverEventListener2_t *other)
{
   if (other != 0)
   {
      apx_serverEventListener2_t *self = (apx_serverEventListener2_t*) malloc(sizeof(apx_serverEventListener2_t));
      if (self != 0)
      {
         *self = *other;
      }
      return self;
   }
   return (apx_serverEventListener2_t*) 0;
}

void apx_serverEventListener2_delete(apx_serverEventListener2_t *self)
{
   if (self != 0)
   {
      free(self);
   }
}

void apx_serverEventListener2_vdelete(void *arg)
{
   apx_serverEventListener2_delete((apx_serverEventListener2_t*) arg);
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


