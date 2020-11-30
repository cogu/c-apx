/*****************************************************************************
* \file      port_attributes.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree APX port attributes
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
#include "apx/port_attributes.h"



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portAttributes_create(apx_portAttributes_t *self, const char *attributeString)
{
   if (self != 0)
   {
      self->isFinalized = false;
      self->isParameter = false;
      self->isQueued = false;
      self->isDynamic = false;
      self->dynLen = 0u;
      self->queueLen = 0u;
      self->initValue = (dtl_dv_t*) 0;
      self->properInitValue = (dtl_dv_t*) 0;
      self->rawString = 0;
      if (attributeString != 0)
      {
         self->rawString = STRDUP(attributeString);
         if (self->rawString == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portAttributes_destroy(apx_portAttributes_t *self)
{
   if (self != 0)
   {
      if (self->rawString != 0)
      {
         free(self->rawString);
      }
      if (self->initValue != 0)
      {
         dtl_dec_ref(self->initValue);
      }
      if (self->properInitValue != 0)
      {
         dtl_dec_ref(self->properInitValue);
      }
   }
}

apx_portAttributes_t* apx_portAttributes_new(const char *attr)
{
   apx_portAttributes_t *self = (apx_portAttributes_t*) 0;
   self = (apx_portAttributes_t*) malloc(sizeof(apx_portAttributes_t));
   if (self != 0)
   {
      apx_error_t rc = apx_portAttributes_create(self, attr);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = (apx_portAttributes_t*) 0;
      }
   }
   return self;
}

void apx_portAttributes_delete(apx_portAttributes_t *self)
{
   if (self != 0)
   {
      apx_portAttributes_destroy(self);
      free(self);
   }
}

void apx_portAttributes_vdelete(void *arg)
{
   apx_portAttributes_delete((apx_portAttributes_t*) arg);
}

void apx_portAttributes_clearInitValue(apx_portAttributes_t *self)
{
   if ( (self != 0) )
   {
      if (self->initValue != 0)
      {
         dtl_dec_ref(self->initValue);
         self->initValue = (dtl_dv_t*) 0;
      }
      if (self->properInitValue != 0)
      {
         dtl_dec_ref(self->properInitValue);
         self->properInitValue = (dtl_dv_t*) 0;
      }
   }
}

dtl_dv_t *apx_portAttributes_getProperInitValue(apx_portAttributes_t *self)
{
   if (self != 0)
   {
      return self->properInitValue;
   }
   return (dtl_dv_t *) 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


