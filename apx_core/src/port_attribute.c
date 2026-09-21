/*****************************************************************************
* \file      port_attribute.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree: APX port attributes
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
void apx_port_attributes_create(apx_port_attributes_t *self)
{
   if (self != NULL)
   {
      self->is_parameter = false;
      self->queue_length = 0u;
      self->init_value = NULL;
   }
}

void apx_port_attributes_destroy(apx_port_attributes_t *self)
{
   if (self != NULL)
   {
      if (self->init_value != NULL)
      {
         dtl_dec_ref(self->init_value);
      }
   }
}

apx_port_attributes_t* apx_port_attributes_new(void)
{
   apx_port_attributes_t *self = NULL;
   self = (apx_port_attributes_t*) malloc(sizeof(apx_port_attributes_t));
   if (self != NULL)
   {
      apx_port_attributes_create(self);
   }
   return self;
}

void apx_port_attributes_delete(apx_port_attributes_t *self)
{
   if (self != NULL)
   {
      apx_port_attributes_destroy(self);
      free(self);
   }
}

void apx_port_attributes_vdelete(void *arg)
{
   apx_port_attributes_delete((apx_port_attributes_t*) arg);
}

void apx_port_attributes_set_parameter(apx_port_attributes_t* self)
{
   if (self != NULL)
   {
      self->is_parameter = true;
   }
}

bool apx_port_attributes_is_parameter(apx_port_attributes_t* self)
{
   if (self != NULL)
   {
      return self->is_parameter;
   }
   return false;
}

bool apx_port_attributes_is_queued(apx_port_attributes_t* self)
{
   if (self != NULL)
   {
      return self->queue_length > 0u ? true : false;
   }
   return false;
}

void apx_port_attributes_set_queue_length(apx_port_attributes_t* self, uint32_t queue_length)
{
   if (self != NULL)
   {
      self->queue_length = queue_length;
   }
}

uint32_t apx_port_attributes_get_queue_length(apx_port_attributes_t* self)
{
   if (self != NULL)
   {
      return self->queue_length;
   }
   return 0u;
}

bool apx_port_attributes_has_init_value(apx_port_attributes_t* self)
{
   if (self != NULL)
   {
      return self->init_value != NULL ? true : false;
   }
   return false;
}

dtl_dv_t* apx_port_attributes_get_init_value(apx_port_attributes_t* self)
{
   if (self != NULL)
   {
      return self->init_value;
   }
   return NULL;
}

void apx_port_attributes_set_init_value(apx_port_attributes_t* self, dtl_dv_t* init_value)
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


