/*****************************************************************************
* \file      node_data.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Container for dynamic data of an APX node
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
apx_error_t apx_node_data_create(apx_node_data_t *self, apx_node_data_buffers_t *buffers)
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
         apx_node_data_set_checksum_data(self, buffers->checksum_type, &buffers->checksum_data[0]);
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

void apx_node_data_destroy(apx_node_data_t *self)
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

apx_node_data_t *apx_node_data_new(void)
{
   apx_node_data_t *self = (apx_node_data_t*) malloc(sizeof(apx_node_data_t));
   if (self != NULL)
   {
      apx_node_data_create(self, (apx_node_data_buffers_t*) NULL);
   }
   return self;
}

void apx_node_data_delete(apx_node_data_t *self)
{
   if (self != NULL)
   {
      apx_node_data_destroy(self);
      free(self);
   }
}
void apx_node_data_vdelete(void *arg)
{
   apx_node_data_delete((apx_node_data_t*) arg);
}

////////////////// Data API //////////////////
apx_size_t apx_node_data_definition_data_size(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->definition_data_size;
   }
   return 0u;
}
apx_size_t apx_node_data_provide_port_data_size(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->provide_port_data_size;
   }
   return 0u;
}

apx_size_t apx_node_data_require_port_data_size(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->require_port_data_size;
   }
   return 0u;
}

apx_size_t apx_node_data_num_provide_ports(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->num_provide_ports;
   }
   return 0u;
}

apx_size_t apx_node_data_num_require_ports(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->num_require_ports;
   }
   return 0u;
}

apx_error_t apx_node_data_create_definition_data(apx_node_data_t* self, uint8_t const* init_data, apx_size_t data_size)
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

apx_error_t apx_node_data_create_provide_port_data(apx_node_data_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size)
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

apx_error_t apx_node_data_create_require_port_data(apx_node_data_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size)
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

apx_error_t apx_node_data_write_definition_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
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

apx_error_t apx_node_data_write_provide_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
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

apx_error_t apx_node_data_read_provide_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size)
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

apx_error_t apx_node_data_write_require_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
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

apx_error_t apx_node_data_read_require_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size)
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

uint8_t const* apx_node_data_get_definition_data(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->definition_data;
   }
   return NULL;
}

uint8_t const* apx_node_data_get_provide_port_data(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->provide_port_data;
   }
   return NULL;
}

uint8_t const* apx_node_data_get_require_port_data(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->require_port_data;
   }
   return NULL;
}

uint8_t* apx_node_data_take_definition_data_snapshot(apx_node_data_t* self)
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

uint8_t* apx_node_data_take_provide_port_data_snapshot(apx_node_data_t* self)
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

uint8_t* apx_node_data_take_require_port_data_snapshot(apx_node_data_t* self)
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

void apx_node_data_set_checksum_data(apx_node_data_t* self, rmf_digest_type_t checksum_type, uint8_t const* checksum_data)
{
   if ((self != NULL) && (checksum_data))
   {
      self->checksum_type = checksum_type;
      memcpy(&self->checksum_data[0], checksum_data, sizeof(self->checksum_data));
   }
}

rmf_digest_type_t apx_node_data_get_checksum_type(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return self->checksum_type;
   }
   return RMF_DIGEST_TYPE_NONE;
}

const uint8_t* apx_node_data_get_checksum_data(apx_node_data_t const* self)
{
   if (self != NULL)
   {
      return &self->checksum_data[0];
   }
   return NULL;
}

#ifndef APX_EMBEDDED
apx_error_t apx_node_data_create_require_port_connection_count_buffer(apx_node_data_t* self, apx_size_t num_require_ports)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (self->require_port_connection_count != NULL)
      {
         free(self->require_port_connection_count);
      }
      self->num_require_ports = num_require_ports;
      if (num_require_ports > 0u)
      {
         self->require_port_connection_count = (apx_port_count_t*)calloc(num_require_ports, sizeof(apx_port_count_t));
         if (self->require_port_connection_count == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            return APX_MEM_ERROR;
         }
      }
      else
      {
         self->require_port_connection_count = NULL;
      }
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_node_data_create_provide_port_connection_count_buffer(apx_node_data_t* self, apx_size_t num_provide_ports)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (self->provide_port_connection_count != NULL)
      {
         free(self->provide_port_connection_count);
      }
      self->num_provide_ports = num_provide_ports;
      if (num_provide_ports > 0u)
      {
         self->provide_port_connection_count = (apx_port_count_t*)calloc(num_provide_ports, sizeof(apx_port_count_t));
         if (self->provide_port_connection_count == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            return APX_MEM_ERROR;
         }
      }
      else
      {
         self->provide_port_connection_count = NULL;
      }
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#endif

apx_size_t apx_node_data_provide_port_connection_count_data_size(apx_node_data_t const* self)
{
   if ((self != NULL) && (self->provide_port_connection_count != NULL))
   {
      return self->num_provide_ports * (apx_size_t)sizeof(apx_port_count_t);
   }
   return 0u;
}

apx_size_t apx_node_data_require_port_connection_count_data_size(apx_node_data_t const* self)
{
   if ((self != NULL) && (self->require_port_connection_count != NULL))
   {
      return self->num_require_ports * (apx_size_t)sizeof(apx_port_count_t);
   }
   return 0u;
}

apx_port_count_t apx_node_data_get_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id)
{
   if (self != NULL)
   {
      apx_port_count_t count = 0u;
      MUTEX_LOCK(self->lock);
      if ((self->provide_port_connection_count != NULL) && (port_id < self->num_provide_ports))
      {
         count = self->provide_port_connection_count[port_id];
      }
      MUTEX_UNLOCK(self->lock);
      return count;
   }
   return 0u;
}

apx_port_count_t apx_node_data_get_require_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id)
{
   if (self != NULL)
   {
      apx_port_count_t count = 0u;
      MUTEX_LOCK(self->lock);
      if ((self->require_port_connection_count != NULL) && (port_id < self->num_require_ports))
      {
         count = self->require_port_connection_count[port_id];
      }
      MUTEX_UNLOCK(self->lock);
      return count;
   }
   return 0u;
}

apx_error_t apx_node_data_set_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id, apx_port_count_t count)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if ((self->provide_port_connection_count != NULL) && (port_id < self->num_provide_ports))
      {
         self->provide_port_connection_count[port_id] = count;
         MUTEX_UNLOCK(self->lock);
         return APX_NO_ERROR;
      }
      MUTEX_UNLOCK(self->lock);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_node_data_set_require_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id, apx_port_count_t count)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if ((self->require_port_connection_count != NULL) && (port_id < self->num_require_ports))
      {
         self->require_port_connection_count[port_id] = count;
         MUTEX_UNLOCK(self->lock);
         return APX_NO_ERROR;
      }
      MUTEX_UNLOCK(self->lock);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_node_data_inc_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if ((self->provide_port_connection_count != NULL) && (port_id < self->num_provide_ports))
      {
         if (self->provide_port_connection_count[port_id] < APX_PORT_COUNT_MAX)
         {
            self->provide_port_connection_count[port_id]++;
         }
      }
      MUTEX_UNLOCK(self->lock);
   }
}

void apx_node_data_dec_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if ((self->provide_port_connection_count != NULL) && (port_id < self->num_provide_ports))
      {
         if (self->provide_port_connection_count[port_id] > 0u)
         {
            self->provide_port_connection_count[port_id]--;
         }
      }
      MUTEX_UNLOCK(self->lock);
   }
}

void apx_node_data_inc_require_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if ((self->require_port_connection_count != NULL) && (port_id < self->num_require_ports))
      {
         if (self->require_port_connection_count[port_id] < APX_PORT_COUNT_MAX)
         {
            self->require_port_connection_count[port_id]++;
         }
      }
      MUTEX_UNLOCK(self->lock);
   }
}

void apx_node_data_dec_require_port_connection_count(apx_node_data_t* self, apx_port_id_t port_id)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if ((self->require_port_connection_count != NULL) && (port_id < self->num_require_ports))
      {
         if (self->require_port_connection_count[port_id] > 0u)
         {
            self->require_port_connection_count[port_id]--;
         }
      }
      MUTEX_UNLOCK(self->lock);
   }
}

uint32_t apx_node_data_get_port_connections_total(apx_node_data_t* self)
{
   if (self != NULL)
   {
      uint32_t total = 0u;
      apx_size_t i;
      MUTEX_LOCK(self->lock);
      if (self->provide_port_connection_count != NULL)
      {
         for (i = 0u; i < self->num_provide_ports; i++)
         {
            total += (uint32_t)self->provide_port_connection_count[i];
         }
      }
      if (self->require_port_connection_count != NULL)
      {
         for (i = 0u; i < self->num_require_ports; i++)
         {
            total += (uint32_t)self->require_port_connection_count[i];
         }
      }
      MUTEX_UNLOCK(self->lock);
      return total;
   }
   return 0u;
}

apx_error_t apx_node_data_write_provide_port_count_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
{
   if (self != NULL)
   {
      apx_size_t total_size = self->num_provide_ports * (apx_size_t)sizeof(apx_port_count_t);
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > total_size || (self->provide_port_connection_count == NULL))
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(((uint8_t*)self->provide_port_connection_count) + offset, src, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_node_data_read_provide_port_count_data(apx_node_data_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size)
{
   if (self != NULL)
   {
      apx_size_t total_size = self->num_provide_ports * (apx_size_t)sizeof(apx_port_count_t);
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > total_size || (self->provide_port_connection_count == NULL))
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(dest, ((uint8_t*)self->provide_port_connection_count) + offset, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_node_data_write_require_port_count_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size)
{
   if (self != NULL)
   {
      apx_size_t total_size = self->num_require_ports * (apx_size_t)sizeof(apx_port_count_t);
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > total_size || (self->require_port_connection_count == NULL))
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(((uint8_t*)self->require_port_connection_count) + offset, src, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_node_data_read_require_port_count_data(apx_node_data_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size)
{
   if (self != NULL)
   {
      apx_size_t total_size = self->num_require_ports * (apx_size_t)sizeof(apx_port_count_t);
      MUTEX_LOCK(self->lock);
      if (((size_t)offset + size) > total_size || (self->require_port_connection_count == NULL))
      {
         MUTEX_UNLOCK(self->lock);
         return APX_INVALID_ARGUMENT_ERROR;
      }
      memcpy(dest, ((uint8_t*)self->require_port_connection_count) + offset, size);
      MUTEX_UNLOCK(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint8_t* apx_node_data_take_provide_port_count_data_snapshot(apx_node_data_t* self)
{
   if (self != NULL)
   {
      uint8_t* snapshot = NULL;
      MUTEX_LOCK(self->lock);
      apx_size_t total_size = self->num_provide_ports * (apx_size_t)sizeof(apx_port_count_t);
      if (self->provide_port_connection_count != NULL && total_size > 0u)
      {
         snapshot = (uint8_t*)malloc(total_size);
         if (snapshot != NULL)
         {
            memcpy(snapshot, self->provide_port_connection_count, total_size);
         }
      }
      MUTEX_UNLOCK(self->lock);
      return snapshot;
   }
   return NULL;
}

uint8_t* apx_node_data_take_require_port_count_data_snapshot(apx_node_data_t* self)
{
   if (self != NULL)
   {
      uint8_t* snapshot = NULL;
      MUTEX_LOCK(self->lock);
      apx_size_t total_size = self->num_require_ports * (apx_size_t)sizeof(apx_port_count_t);
      if (self->require_port_connection_count != NULL && total_size > 0u)
      {
         snapshot = (uint8_t*)malloc(total_size);
         if (snapshot != NULL)
         {
            memcpy(snapshot, self->require_port_connection_count, total_size);
         }
      }
      MUTEX_UNLOCK(self->lock);
      return snapshot;
   }
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


