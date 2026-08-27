/*****************************************************************************
* \file      node_data.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Container for dynamic data of an APX node
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#include <malloc.h>
#include <assert.h>
#include "apx/node_data.h"
#include "apx/node_instance.h"
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
////////////////// Constructor/Destructor //////////////////
apx_error_t apx_nodeData_create(apx_nodeData_t *self, apx_nodeDataBuffers_t *buffers)
{
   if (self != NULL)
   {
      if (buffers != NULL)
      {
         self->is_weak_ref = true;
         self->definition_data = buffers->definition_data;
         self->require_port_data = buffers->require_port_data;
         self->provide_port_data = buffers->provide_port_data;
         self->definition_data_size = buffers->definition_data_size;
         self->require_port_data_size = buffers->require_port_data_size;
         self->provide_port_data_size = buffers->provide_port_data_size;
         self->require_port_connection_count = buffers->require_port_connection_count;
         self->provide_port_connection_count = buffers->provide_port_connection_count;
         self->num_require_ports = buffers->num_require_ports;
         self->num_provide_ports = buffers->num_provide_ports;
         apx_nodeData_set_checksum_data(self, buffers->checksum_type, &buffers->checksum_data[0]);
      }
      else
      {
         self->is_weak_ref = false;
         self->definition_data = NULL;
         self->require_port_data = NULL;
         self->provide_port_data = NULL;
         self->definition_data_size = 0u;
         self->require_port_data_size = 0u;
         self->provide_port_data_size = 0u;
         self->require_port_connection_count = NULL;
         self->provide_port_connection_count = NULL;
         self->num_require_ports = 0u;
         self->num_provide_ports = 0u;
         self->checksum_type = RMF_DIGEST_TYPE_NONE;
         memset(&self->checksum_data[0], 0, sizeof(self->checksum_data));
      }
#ifndef APX_EMBEDDED
      MUTEX_INIT(self->lock);
#endif
   }
   return APX_NO_ERROR;
}

void apx_nodeData_destroy(apx_nodeData_t *self)
{
   if ( (self != NULL)  )
   {
#ifndef APX_EMBEDDED
      if (!self->is_weak_ref)
      {
         if (self->definition_data != 0)
         {
            free(self->definition_data);
         }
         if (self->require_port_data != 0)
         {
            free(self->require_port_data);
         }
         if (self->provide_port_data != 0)
         {
            free(self->provide_port_data);
         }
         if (self->require_port_connection_count != 0)
         {
            free(self->require_port_connection_count);
         }
         if (self->provide_port_connection_count != 0)
         {
            free(self->provide_port_connection_count);
         }
      }
      MUTEX_DESTROY(self->lock);
#endif
   }
}

apx_nodeData_t *apx_nodeData_new(void)
{
   apx_nodeData_t *self = (apx_nodeData_t*) malloc(sizeof(apx_nodeData_t));
   if (self != 0)
   {
      apx_nodeData_create(self, (apx_nodeDataBuffers_t*) NULL);
   }
   return self;
}

void apx_nodeData_delete(apx_nodeData_t *self)
{
   if (self != 0)
   {
      apx_nodeData_destroy(self);
      free(self);
   }
}
void apx_nodeData_vdelete(void *arg)
{
   apx_nodeData_delete((apx_nodeData_t*) arg);
}

////////////////// Data API //////////////////
apx_size_t apx_nodeData_definition_data_size(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->definition_data_size;
   }
   return 0u;
}
apx_size_t apx_nodeData_provide_port_data_size(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->provide_port_data_size;
   }
   return 0u;
}

apx_size_t apx_nodeData_require_port_data_size(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->require_port_data_size;
   }
   return 0u;
}

apx_size_t apx_nodeData_num_provide_ports(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->num_provide_ports;
   }
   return 0u;
}

apx_size_t apx_nodeData_num_require_ports(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->num_require_ports;
   }
   return 0u;
}

apx_error_t apx_nodeData_create_definition_data(apx_nodeData_t* self, uint8_t const* init_data, apx_size_t data_size)
{
   if (self != NULL)
   {
      if (self->is_weak_ref)
      {
         return APX_UNSUPPORTED_ERROR;
      }
      if (data_size > 0u)
      {
         MUTEX_LOCK(self->lock);
         self->definition_data = (uint8_t*)malloc(data_size);
         if (self->definition_data == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            return APX_MEM_ERROR;
         }
         self->definition_data_size = data_size;
         if (init_data != NULL)
         {
            memcpy(self->definition_data, init_data, data_size);
         }
         else
         {
            memset(self->definition_data, 0, data_size);
         }
         MUTEX_UNLOCK(self->lock);
         return APX_NO_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_create_provide_port_data(apx_nodeData_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size)
{
   if (self != NULL)
   {
      if (self->is_weak_ref)
      {
         return APX_UNSUPPORTED_ERROR;
      }
      if (data_size > 0u)
      {
         MUTEX_LOCK(self->lock);
         self->provide_port_data = (uint8_t*)malloc(data_size);
         if (self->provide_port_data == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            return APX_MEM_ERROR;
         }
         self->num_provide_ports = num_ports;
         self->provide_port_data_size = data_size;
         if (init_data != NULL)
         {
            memcpy(self->provide_port_data, init_data, data_size);
         }
         else
         {
            memset(self->provide_port_data, 0, data_size);
         }
         MUTEX_UNLOCK(self->lock);
         return APX_NO_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_create_require_port_data(apx_nodeData_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size)
{
   if (self != NULL)
   {
      if (self->is_weak_ref)
      {
         return APX_UNSUPPORTED_ERROR;
      }
      if (data_size > 0u)
      {
         MUTEX_LOCK(self->lock);
         self->require_port_data = (uint8_t*)malloc(data_size);
         if (self->require_port_data == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            return APX_MEM_ERROR;
         }
         self->num_require_ports = num_ports;
         self->require_port_data_size = data_size;
         if (init_data != NULL)
         {
            memcpy(self->require_port_data, init_data, data_size);
         }
         else
         {
            memset(self->require_port_data, 0, data_size);
         }
         MUTEX_UNLOCK(self->lock);
         return APX_NO_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_write_definition_data(apx_nodeData_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > self->definition_data_size)
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(self->definition_data + offset, src, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_write_provide_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > self->provide_port_data_size)
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(self->provide_port_data + offset, src, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_read_provide_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > self->provide_port_data_size)
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(dest, self->provide_port_data + offset, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_write_require_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > self->require_port_data_size)
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(self->require_port_data + offset, src, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeData_read_require_port_data(apx_nodeData_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > self->require_port_data_size)
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(dest, self->require_port_data + offset, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint8_t const* apx_nodeData_get_definition_data(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->definition_data;
   }
   return NULL;
}

uint8_t const* apx_nodeData_get_provide_port_data(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->provide_port_data;
   }
   return NULL;
}

uint8_t const* apx_nodeData_get_require_port_data(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->require_port_data;
   }
   return NULL;
}

uint8_t* apx_nodeData_take_definition_data_snapshot(apx_nodeData_t* self)
{
   if (self != NULL)
   {
      uint8_t* snapshot = NULL;
      MUTEX_LOCK(self->lock);
      if (self->definition_data != NULL)
      {
         snapshot = (uint8_t*)malloc(self->definition_data_size);
         if (snapshot != NULL)
         {
            memcpy(snapshot, self->definition_data, self->definition_data_size);
         }
      }
      MUTEX_UNLOCK(self->lock);
      return snapshot;
   }
   return NULL;
}

uint8_t* apx_nodeData_take_provide_port_data_snapshot(apx_nodeData_t* self)
{
   if (self != NULL)
   {
      uint8_t* snapshot = NULL;
      MUTEX_LOCK(self->lock);
      if (self->provide_port_data != NULL)
      {
         snapshot = (uint8_t*)malloc(self->provide_port_data_size);
         if (snapshot != NULL)
         {
            memcpy(snapshot, self->provide_port_data, self->provide_port_data_size);
         }
      }
      MUTEX_UNLOCK(self->lock);
      return snapshot;
   }
   return NULL;
}

uint8_t* apx_nodeData_take_require_port_data_snapshot(apx_nodeData_t* self)
{
   if (self != NULL)
   {
      uint8_t* snapshot = NULL;
      MUTEX_LOCK(self->lock);
      if (self->require_port_data != NULL)
      {
         snapshot = (uint8_t*)malloc(self->require_port_data_size);
         if (snapshot != NULL)
         {
            memcpy(snapshot, self->require_port_data, self->require_port_data_size);
         }
      }
      MUTEX_UNLOCK(self->lock);
      return snapshot;
   }
   return NULL;
}

void apx_nodeData_set_checksum_data(apx_nodeData_t* self, rmf_digestType_t checksum_type, uint8_t const* checksum_data)
{
   if ((self != NULL) && (checksum_data))
   {
      self->checksum_type = checksum_type;
      memcpy(&self->checksum_data[0], checksum_data, sizeof(self->checksum_data));
   }
}

rmf_digestType_t apx_nodeData_get_checksum_type(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return self->checksum_type;
   }
   return RMF_DIGEST_TYPE_NONE;
}

const uint8_t* apx_nodeData_get_checksum_data(apx_nodeData_t const* self)
{
   if (self != NULL)
   {
      return &self->checksum_data[0];
   }
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


