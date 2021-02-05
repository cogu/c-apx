/*****************************************************************************
* \file      file_manager_shared.c
* \author    Conny Gustafsson
* \date      2020-01-23
* \brief     APX Filemanager shared data
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#include <assert.h>
#include "adt_list.h"
#include "apx/file_manager_shared.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void set_connection(apx_fileManagerShared_t* self, apx_connectionInterface_t const* connection);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_fileManagerShared_create(apx_fileManagerShared_t* self, apx_connectionInterface_t const* parent_connection, apx_allocator_t* allocator)
{
   if (self != NULL)
   {
      set_connection(self, parent_connection);
      self->connection_id = APX_INVALID_CONNECTION_ID;
      self->is_connected = false;
      self->allocator = allocator;
      apx_fileMap_create(&self->local_file_map, false);
      apx_fileMap_create(&self->remote_file_map, true);
      MUTEX_INIT(self->lock);
   }
}

void apx_fileManagerShared_destroy(apx_fileManagerShared_t *self)
{
   if (self != NULL)
   {
      apx_fileMap_destroy(&self->local_file_map);
      apx_fileMap_destroy(&self->remote_file_map);
      MUTEX_DESTROY(self->lock);
   }
}


apx_file_t* apx_fileManagerShared_create_local_file(apx_fileManagerShared_t* self, const rmf_fileInfo_t* file_info)
{
   if ( (self != NULL) && (file_info != NULL) )
   {
      apx_file_t* file = NULL;
      MUTEX_LOCK(self->lock);
      file = apx_fileMap_create_file(&self->local_file_map, file_info);
      MUTEX_UNLOCK(self->lock);
      return file;
   }
   return NULL;
}

apx_file_t* apx_fileManagerShared_create_remote_file(apx_fileManagerShared_t* self, const rmf_fileInfo_t* file_info)
{
   if ((self != NULL) && (file_info != NULL))
   {
      apx_file_t* file = NULL;
      MUTEX_LOCK(self->lock);
      file = apx_fileMap_create_file(&self->remote_file_map, file_info);
      MUTEX_UNLOCK(self->lock);
      return file;
   }
   return NULL;
}

int32_t apx_fileManagerShared_get_num_local_files(apx_fileManagerShared_t* self)
{
   if (self != NULL)
   {
      int32_t retval;
      MUTEX_LOCK(self->lock);
      retval = apx_fileMap_length(&self->local_file_map);
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_fileManagerShared_get_num_remote_files(apx_fileManagerShared_t* self)
{
   if (self != NULL)
   {
      int32_t retval;
      MUTEX_LOCK(self->lock);
      retval = apx_fileMap_length(&self->remote_file_map);
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return -1;
}

apx_file_t* apx_fileManagerShared_find_local_file_by_name(apx_fileManagerShared_t* self, const char* name)
{
   if ( (self != NULL) && (name != NULL) )
   {
      apx_file_t *file;
      MUTEX_LOCK(self->lock);
      file = apx_fileMap_find_by_name(&self->local_file_map, name);
      MUTEX_UNLOCK(self->lock);
      return file;
   }
   return NULL;
}

apx_file_t* apx_fileManagerShared_find_remote_file_by_name(apx_fileManagerShared_t* self, const char* name)
{
   if ( (self != NULL) && (name != NULL) )
   {
      apx_file_t *localFile;
      MUTEX_LOCK(self->lock);
      localFile = apx_fileMap_find_by_name(&self->remote_file_map, name);
      MUTEX_UNLOCK(self->lock);
      return localFile;
   }
   return NULL;
}

apx_file_t* apx_fileManagerShared_find_file_by_address(apx_fileManagerShared_t* self, uint32_t address)
{
   if ( (self != NULL) && (address != RMF_INVALID_ADDRESS))
   {
      apx_file_t *file;
      uint32_t address_without_flags = address & RMF_ADDRESS_MASK_INTERNAL;
      MUTEX_LOCK(self->lock);
      if ( (address & RMF_REMOTE_ADDRESS_BIT) != 0u)
      {
         file = apx_fileMap_find_by_address(&self->remote_file_map, address_without_flags);
      }
      else
      {
         file = apx_fileMap_find_by_address(&self->local_file_map, address_without_flags);
      }
      MUTEX_UNLOCK(self->lock);
      return file;
   }
   return (apx_file_t*) NULL;
}


void apx_fileManagerShared_set_connection_id(apx_fileManagerShared_t* self, uint32_t connection_id)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      self->connection_id = connection_id;
      MUTEX_UNLOCK(self->lock);
   }
}

uint32_t apx_fileManagerShared_get_connection_id(apx_fileManagerShared_t* self)
{
   if (self != NULL)
   {
      uint32_t retval;
      MUTEX_LOCK(self->lock);
      retval = self->connection_id;
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return APX_INVALID_CONNECTION_ID;
}

int32_t apx_fileManagerShared_copy_local_file_info(apx_fileManagerShared_t* self, adt_ary_t* array)
{
   if ( (self != NULL) && (array != NULL) )
   {
      int32_t num_items = 0;
      adt_list_t const* list;
      adt_list_elem_t* iter;
      MUTEX_LOCK(self->lock);
      list = apx_fileMap_get_list(&self->local_file_map);
      assert(list != NULL);
      iter = adt_list_iter_first(list);
      while (iter != NULL)
      {
         apx_file_t* file = (apx_file_t*)iter->pItem;
         assert(file != NULL);
         rmf_fileInfo_t* file_info = rmf_fileInfo_clone(apx_file_get_file_info(file));
         if (file_info == NULL)
         {
            //out of memory error occured
            MUTEX_UNLOCK(self->lock);
            return -1;
         }
         adt_ary_push(array, file_info);
         num_items++;
         iter = adt_list_iter_next(iter);
      }
      MUTEX_UNLOCK(self->lock);
      return num_items;
   }
   return -1;
}

void apx_fileManagerShared_connected(apx_fileManagerShared_t* self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      self->is_connected = true;
      MUTEX_UNLOCK(self->lock);
   }
}

void apx_fileManagerShared_disconnected(apx_fileManagerShared_t* self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      self->is_connected = false;
      MUTEX_UNLOCK(self->lock);
#if APX_DEBUG_ENABLE
      printf("[%u] Disabled transmit handler\n", (unsigned int) self->connection_id);
#endif
   }
}

bool apx_fileManagerShared_is_connected(apx_fileManagerShared_t* self)
{
   if (self != NULL)
   {
      bool retval;
      MUTEX_LOCK(self->lock);
      retval = self->is_connected;
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return false;
}


apx_connectionInterface_t const* apx_fileManagerShared_connection(apx_fileManagerShared_t const* self)
{
   if (self != NULL)
   {
      return &self->parent_connection;
   }
   return NULL;
}

apx_allocator_t* apx_fileManagerShared_allocator(apx_fileManagerShared_t const* self)
{
   if (self != NULL)
   {
      return self->allocator;
   }
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void set_connection(apx_fileManagerShared_t* self, apx_connectionInterface_t const* connection)
{
   assert(self != NULL);
   if (connection != NULL)
   {
      memcpy(&self->parent_connection, connection, sizeof(apx_connectionInterface_t));
   }
   else
   {
      memset(&self->parent_connection, 0, sizeof(apx_connectionInterface_t));
   }
}

