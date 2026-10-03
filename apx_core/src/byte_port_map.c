/*****************************************************************************
* \file      byte_port_map.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map using contiguous partition indexing
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <assert.h>
#include "apx/byte_port_map.h"
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_byte_port_map_build(apx_byte_port_map_t *self, apx_port_instance_t const* port_instance_list, apx_size_t num_ports, apx_size_t map_len);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_byte_port_map_create(apx_byte_port_map_t* self, apx_size_t total_size, apx_port_instance_t const* port_instance_list, apx_size_t num_ports)
{
   apx_error_t retval = APX_INVALID_ARGUMENT_ERROR;
   if ( (self != NULL) && (port_instance_list != NULL) && (num_ports > 0u) && (total_size > 0u) )
   {
      self->partition_type = APX_PARTITION_U16;
      retval = apx_byte_port_map_build(self, port_instance_list, num_ports, total_size);
   }
   return retval;
}

void apx_byte_port_map_destroy(apx_byte_port_map_t *self)
{
   if (self != NULL)
   {
      if (self->partition_type == APX_PARTITION_U16)
      {
         adt_u16_partition_destroy(&self->partition.u16_part);
      }
      else
      {
         adt_u32_partition_destroy(&self->partition.u32_part);
      }
   }
}

apx_byte_port_map_t* apx_byte_port_map_new(apx_size_t total_size, apx_port_instance_t const* port_instance_list, apx_size_t num_ports, apx_error_t* error_code)
{
   apx_byte_port_map_t *self = (apx_byte_port_map_t*) malloc(sizeof(apx_byte_port_map_t));
   if (self != NULL)
   {
      apx_error_t result = apx_byte_port_map_create(self, total_size, port_instance_list, num_ports);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
      if (error_code != NULL)
      {
         *error_code = result;
      }
   }
   return self;
}

void apx_byte_port_map_delete(apx_byte_port_map_t *self)
{
   if (self != NULL)
   {
      apx_byte_port_map_destroy(self);
      free(self);
   }
}

apx_port_id_t apx_byte_port_map_lookup(const apx_byte_port_map_t *self, uint32_t offset)
{
   if (self != NULL)
   {
      int32_t index;
      if (self->partition_type == APX_PARTITION_U16)
      {
         if (offset > UINT16_MAX)
         {
            return APX_INVALID_PORT_ID;
         }
         index = adt_u16_partition_find(&self->partition.u16_part, (uint16_t)offset);
      }
      else
      {
         index = adt_u32_partition_find(&self->partition.u32_part, offset);
      }

      if (index >= 0)
      {
         return (apx_port_id_t)index;
      }
   }
   return APX_INVALID_PORT_ID;
}

apx_size_t apx_byte_port_map_length(const apx_byte_port_map_t *self)
{
   if (self != NULL)
   {
      if (self->partition_type == APX_PARTITION_U16)
      {
         return (apx_size_t)adt_u16_partition_total_size(&self->partition.u16_part);
      }
      else
      {
         return (apx_size_t)adt_u32_partition_total_size(&self->partition.u32_part);
      }
   }
   return 0u;
}

uint8_t apx_byte_port_map_type(const apx_byte_port_map_t *self)
{
   return (self != NULL) ? self->partition_type : APX_PARTITION_U16;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_byte_port_map_build(apx_byte_port_map_t* self, apx_port_instance_t const* port_instance_list, apx_size_t num_ports, apx_size_t map_len)
{
   if ( (self != NULL) && (port_instance_list != NULL) && (num_ports > 0u) && (map_len > 0u) )
   {
      apx_port_id_t port_id;
      adt_error_t adt_err;

      if (map_len <= UINT16_MAX)
      {
         self->partition_type = APX_PARTITION_U16;
         adt_u16_partition_create(&self->partition.u16_part);
         adt_err = adt_u16_partition_reserve(&self->partition.u16_part, (uint32_t)num_ports);
         if (adt_err != ADT_NO_ERROR)
         {
            return (adt_err == ADT_MEM_ERROR) ? APX_MEM_ERROR : APX_INTERNAL_ERROR;
         }

         for (port_id = 0; port_id < (apx_port_id_t)num_ports; port_id++)
         {
            apx_size_t const pack_len = (apx_size_t)apx_port_instance_data_size(&port_instance_list[port_id]);
            adt_err = adt_u16_partition_append(&self->partition.u16_part, (uint16_t)pack_len);
            if (adt_err != ADT_NO_ERROR)
            {
               adt_u16_partition_destroy(&self->partition.u16_part);
               return (adt_err == ADT_MEM_ERROR) ? APX_MEM_ERROR : APX_LENGTH_ERROR;
            }
         }

         if (adt_u16_partition_total_size(&self->partition.u16_part) != (uint16_t)map_len)
         {
            adt_u16_partition_destroy(&self->partition.u16_part);
            return APX_LENGTH_ERROR;
         }
      }
      else
      {
         self->partition_type = APX_PARTITION_U32;
         adt_u32_partition_create(&self->partition.u32_part);
         adt_err = adt_u32_partition_reserve(&self->partition.u32_part, (uint32_t)num_ports);
         if (adt_err != ADT_NO_ERROR)
         {
            return (adt_err == ADT_MEM_ERROR) ? APX_MEM_ERROR : APX_INTERNAL_ERROR;
         }

         for (port_id = 0; port_id < (apx_port_id_t)num_ports; port_id++)
         {
            apx_size_t const pack_len = (apx_size_t)apx_port_instance_data_size(&port_instance_list[port_id]);
            adt_err = adt_u32_partition_append(&self->partition.u32_part, (uint32_t)pack_len);
            if (adt_err != ADT_NO_ERROR)
            {
               adt_u32_partition_destroy(&self->partition.u32_part);
               return (adt_err == ADT_MEM_ERROR) ? APX_MEM_ERROR : APX_LENGTH_ERROR;
            }
         }

         if (adt_u32_partition_total_size(&self->partition.u32_part) != (uint32_t)map_len)
         {
            adt_u32_partition_destroy(&self->partition.u32_part);
            return APX_LENGTH_ERROR;
         }
      }

      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
