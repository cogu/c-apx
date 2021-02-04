/*****************************************************************************
* \file      port_attribute.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree: APX port attributes
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
#include "apx/port_attribute.h"
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
void apx_portAttributes_create(apx_portAttributes_t *self)
{
   if (self != NULL)
   {
      self->is_parameter = false;
      self->queue_length = 0u;
      self->init_value = NULL;
   }
}

void apx_portAttributes_destroy(apx_portAttributes_t *self)
{
   if (self != NULL)
   {
      if (self->init_value != NULL)
      {
         dtl_dec_ref(self->init_value);
      }
   }
}

apx_portAttributes_t* apx_portAttributes_new(void)
{
   apx_portAttributes_t *self = (apx_portAttributes_t*) 0;
   self = (apx_portAttributes_t*) malloc(sizeof(apx_portAttributes_t));
   if (self != NULL)
   {
      apx_portAttributes_create(self);
   }
   return self;
}

void apx_portAttributes_delete(apx_portAttributes_t *self)
{
   if (self != NULL)
   {
      apx_portAttributes_destroy(self);
      free(self);
   }
}

void apx_portAttributes_vdelete(void *arg)
{
   apx_portAttributes_delete((apx_portAttributes_t*) arg);
}

void apx_portAttributes_set_parameter(apx_portAttributes_t* self)
{
   if (self != NULL)
   {
      self->is_parameter = true;
   }
}

bool apx_portAttributes_is_parameter(apx_portAttributes_t* self)
{
   if (self != NULL)
   {
      return self->is_parameter;
   }
   return false;
}

bool apx_portAttributes_is_queued(apx_portAttributes_t* self)
{
   if (self != NULL)
   {
      return self->queue_length > 0u ? true : false;
   }
   return false;
}

void apx_portAttributes_set_queue_length(apx_portAttributes_t* self, uint32_t queue_length)
{
   if (self != NULL)
   {
      self->queue_length = queue_length;
   }
}

uint32_t apx_portAttributes_get_queue_length(apx_portAttributes_t* self)
{
   if (self != NULL)
   {
      return self->queue_length;
   }
   return 0u;
}

bool apx_portAttributes_has_init_value(apx_portAttributes_t* self)
{
   if (self != NULL)
   {
      return self->init_value != NULL ? true : false;
   }
   return false;
}

dtl_dv_t* apx_portAttributes_get_init_value(apx_portAttributes_t* self)
{
   if (self != NULL)
   {
      return self->init_value;
   }
   return NULL;
}

void apx_portAttributes_set_init_value(apx_portAttributes_t* self, dtl_dv_t* init_value)
{
   if (self != NULL)
   {
      if (self->init_value != NULL)
      {
         dtl_dec_ref(self->init_value);
         self->init_value = init_value;
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


