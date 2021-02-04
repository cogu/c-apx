/*****************************************************************************
* \file      byte_port_map.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
      retval = APX_NO_ERROR;
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
   if (self != 0)
   {
      apx_bytePortMap_destroy(self);
      free(self);
   }
}


apx_portId_t apx_bytePortMap_lookup(const apx_bytePortMap_t *self, uint32_t offset)
{
   if ( (self != 0) && (offset >= 0) && (offset < self->map_len) )
   {
      return self->map_data[offset];
   }
   return APX_INVALID_PORT_ID;
}

apx_size_t apx_bytePortMap_length(const apx_bytePortMap_t *self)
{
   if (self != 0)
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
      apx_portId_t* next;
      apx_portId_t* end;
      self->map_data = (apx_portId_t*) malloc(map_len * sizeof(apx_portId_t));
      if (self->map_data == NULL)
      {
         return APX_MEM_ERROR;
      }
      self->map_len = map_len;
      next = self->map_data;
      end = next + self->map_len;
      for (port_id =0; port_id < num_ports; port_id++)
      {
         apx_size_t i;
         apx_size_t pack_len = (apx_size_t)apx_portInstance_data_size(&port_instance_list[port_id]);
         for(i=0u; i < pack_len; i++)
         {
            next[i] = port_id;
         }
         next += pack_len;
         assert(next <= end);
      }
      assert(next == end);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
