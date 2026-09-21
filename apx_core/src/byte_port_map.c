/*****************************************************************************
* \file      byte_port_map.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <assert.h>
#include "apx/byte_port_map.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_bytePortMap_build(apx_bytePortMap_t *self, apx_portInstance_t const* port_instance_list, apx_size_t num_ports, apx_size_t map_len);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////




apx_portId_t apx_bytePortMap_lookup(const apx_bytePortMap_t* self, uint32_t offset);
apx_size_t apx_bytePortMap_length(const apx_bytePortMap_t* self);

apx_error_t apx_bytePortMap_create(apx_bytePortMap_t* self, apx_size_t total_size, apx_portInstance_t const* port_instance_list, apx_size_t num_ports)
{
   apx_error_t retval = APX_INVALID_ARGUMENT_ERROR;
   if ( (self != NULL) && (port_instance_list != NULL) && (num_ports > 0u) && (total_size > 0u) )
   {
      self->map_data = (apx_portId_t*) NULL;
      self->map_len = 0;
      retval = apx_bytePortMap_build(self, port_instance_list, num_ports, total_size);
   }
   return retval;
}

void apx_bytePortMap_destroy(apx_bytePortMap_t *self)
{
   if ( (self != NULL) && (self->map_data != NULL) )
   {
      free(self->map_data);
   }
}

apx_bytePortMap_t* apx_bytePortMap_new(apx_size_t total_size, apx_portInstance_t const* port_instance_list, apx_size_t num_ports, apx_error_t* errorCode)
{
   apx_bytePortMap_t *self = (apx_bytePortMap_t*) malloc(sizeof(apx_bytePortMap_t));
   if (self != NULL)
   {
      apx_error_t result = apx_bytePortMap_create(self, total_size, port_instance_list, num_ports);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = (apx_bytePortMap_t*) NULL;
      }
      if (errorCode != 0)
      {
         *errorCode = result;
      }
   }
   return self;
}

void apx_bytePortMap_delete(apx_bytePortMap_t *self)
{
   if (self != NULL)
   {
      apx_bytePortMap_destroy(self);
      free(self);
   }
}


apx_portId_t apx_bytePortMap_lookup(const apx_bytePortMap_t *self, uint32_t offset)
{
   if ( (self != NULL) && (offset < self->map_len) )
   {
      return self->map_data[offset];
   }
   return APX_INVALID_PORT_ID;
}

apx_size_t apx_bytePortMap_length(const apx_bytePortMap_t *self)
{
   if (self != NULL)
   {
      return self->map_len;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_bytePortMap_build(apx_bytePortMap_t* self, apx_portInstance_t const* port_instance_list, apx_size_t num_ports, apx_size_t map_len)
{
   if ( (self != NULL) && (port_instance_list != NULL) && (num_ports > 0u) && (map_len > 0u) )
   {
      apx_portId_t port_id;
      apx_size_t offset = 0u;
      apx_portId_t* map_data = (apx_portId_t*) malloc(map_len * sizeof(apx_portId_t));
      if (map_data == NULL)
      {
         return APX_MEM_ERROR;
      }
      for (port_id = 0; port_id < num_ports; port_id++)
      {
         apx_size_t i;
         apx_size_t pack_len = (apx_size_t)apx_portInstance_data_size(&port_instance_list[port_id]);
         if (pack_len > (map_len - offset))
         {
            free(map_data);
            return APX_LENGTH_ERROR;
         }
         for (i = 0u; (i < pack_len) && ((offset + i) < map_len); i++)
         {
            map_data[offset + i] = port_id;
         }
         offset += pack_len;
      }
      if (offset != map_len)
      {
         free(map_data);
         return APX_LENGTH_ERROR;
      }
      self->map_data = map_data;
      self->map_len = map_len;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

