/*****************************************************************************
* \file      file_map.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     File map data structure
*
* Copyright (c) 2017-2021 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include "apx/file_map.h"
#include "apx/remotefile.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static adt_list_elem_t* auto_assign_address(apx_fileMap_t* self, apx_file_t* file);
static adt_list_elem_t* find_last_element_of_type(apx_fileMap_t* self, apx_fileType_t file_type);
static adt_list_elem_t* find_next_available_position(apx_fileMap_t* self, apx_file_t* file);
static bool insert_item(apx_fileMap_t* self, apx_file_t* file, adt_list_elem_t* iterator_left);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


void apx_fileMap_create(apx_fileMap_t* self, bool is_remote)
{
   if (self != NULL)
   {
      self->is_remote_map = is_remote;
      adt_list_create(&self->file_list, apx_file_vdelete);
      self->last_file = (apx_file_t*) 0;
   }
}
void apx_fileMap_destroy(apx_fileMap_t *self)
{
   if (self != NULL)
   {
      adt_list_destroy(&self->file_list);
   }
}

bool apx_fileMap_is_remote(apx_fileMap_t const* self)
{
   if (self != NULL)
   {
      return self->is_remote_map;
   }
   return false;
}

apx_file_t* apx_fileMap_create_file(apx_fileMap_t* self, rmf_fileInfo_t const* file_info)
{
   if ( (self != NULL) && (file_info != NULL))
   {
      apx_file_t* file = apx_file_new(file_info);
      if (file == NULL)
      {
         return NULL;
      }
      adt_list_elem_t* iterator_left = NULL;
      if (apx_file_get_address_without_flags(file) == RMF_INVALID_ADDRESS)
      {
         iterator_left = auto_assign_address(self, file);
         if (iterator_left == NULL)
         {
            iterator_left = find_next_available_position(self, file);
         }
      }
      else
      {
         iterator_left = find_next_available_position(self, file);
      }
      if (self->is_remote_map)
      {
         uint32_t tmp = apx_file_get_address(file);
         apx_file_set_address(file, tmp | RMF_REMOTE_ADDRESS_BIT);
      }
      bool result = insert_item(self, file, iterator_left);
      if (!result)
      {
         apx_file_delete(file);
         file = NULL;
      }
      return file;

   }
   return NULL;
}

apx_error_t apx_fileMap_remove_file(apx_fileMap_t* self, apx_file_t* file)
{
   if ( (self != NULL) && (file != NULL) )
   {
      self->last_file = (apx_file_t*)NULL;
      bool result = adt_list_remove(&self->file_list, file);
      return result ? APX_NO_ERROR : APX_NOT_FOUND_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_file_t* apx_fileMap_find_by_address(apx_fileMap_t* self, uint32_t address)
{
   apx_file_t *retval = NULL;
   if (self != NULL)
   {
      if (self->last_file != NULL)
      {
         if (apx_file_address_in_range(self->last_file, address))
         {
            retval = self->last_file;
         }
      }
      if (retval == NULL)
      {
         adt_list_elem_t *pIter = adt_list_iter_first(&self->file_list);
         while(pIter != NULL)
         {
            apx_file_t *file = (apx_file_t*) pIter->pItem;
            assert(file != NULL);
            if (apx_file_address_in_range(file, address))
            {
               self->last_file = retval = file;
               break;
            }
            pIter = adt_list_iter_next(pIter);
         }
      }
   }
   return retval;
}

apx_file_t* apx_fileMap_find_by_name(apx_fileMap_t* self, const char* name)
{
   apx_file_t *retval = NULL;
   if (self != NULL)
   {
      if (self->last_file != NULL)
      {
         if ((strcmp(apx_file_get_name(self->last_file), name) == 0))
         {
            retval = self->last_file;
         }
      }
      if (retval == NULL)
      {
         adt_list_elem_t* pIter = adt_list_iter_first(&self->file_list);
         while (pIter != NULL)
         {
            apx_file_t* file = (apx_file_t*)pIter->pItem;
            assert(file != NULL);
            if ((strcmp(apx_file_get_name(file), name) == 0))
            {
               self->last_file = retval = file;
               break;
            }
            pIter = adt_list_iter_next(pIter);
         }
      }
   }
   return retval;
}

int32_t apx_fileMap_length(apx_fileMap_t const* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->file_list);
   }
   return -1;
}

adt_list_t const* apx_fileMap_get_list(apx_fileMap_t const* self)
{
   if (self != NULL)
   {
      return &self->file_list;
   }
   return (adt_list_t*) 0;
}

bool apx_fileMap_exist(apx_fileMap_t const* self, apx_file_t* file)
{
   if (self != NULL)
   {
      adt_list_elem_t *pIter = adt_list_iter_first(&self->file_list);
      while(pIter != NULL)
      {
         if (file == (apx_file_t*) pIter->pItem)
         {
            return true;
         }
         pIter = adt_list_iter_next(pIter);
      }
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static adt_list_elem_t* auto_assign_address(apx_fileMap_t* self, apx_file_t* file)
{
   uint32_t start_address = 0u;
   uint32_t alignment = 0u;
   apx_fileType_t file_type = apx_file_get_apx_file_type(file);
   switch (file_type)
   {
   case APX_DEFINITION_FILE_TYPE:
      start_address = APX_DEFINITION_ADDRESS_START;
      alignment = APX_DEFINITION_ADDRESS_ALIGNMENT;
      break;
   case APX_PROVIDE_PORT_DATA_FILE_TYPE:
   case APX_REQUIRE_PORT_DATA_FILE_TYPE:
      alignment = APX_PORT_DATA_ADDRESS_ALIGNMENT;
      break;
   case APX_PROVIDE_PORT_COUNT_FILE_TYPE:
   case APX_REQUIRE_PORT_COUNT_FILE_TYPE:
      start_address = APX_PORT_COUNT_ADDRESS_START;
      alignment = APX_PORT_COUNT_ADDRESS_ALIGNMENT;
      break;
   default:
      start_address = APX_USER_DEFINED_ADDRESS_START;
      alignment = APX_USER_DEFINED_ADDRESS_ALIGNMENT;
   }
   adt_list_elem_t* iterator_left = find_last_element_of_type(self, file_type);
   if (iterator_left == NULL)
   {
      apx_file_set_address(file, start_address);
   }
   else
   {
      uint32_t address = start_address;
      uint32_t const end_address = apx_file_get_end_address_without_flags((apx_file_t*)iterator_left->pItem);
      assert(alignment != 0);
      if ((alignment & (alignment - 1)) == 0)
      {
         //alignment is a power of 2, use quick method
         address = (end_address + (alignment - 1)) & (~(alignment - 1));
      }
      else
      {
         //Use slower method
         while (address < end_address)
         {
            address += alignment;
         }
      }
      apx_file_set_address(file, address);
   }
   return iterator_left;
}

static adt_list_elem_t* find_last_element_of_type(apx_fileMap_t* self, apx_fileType_t file_type)
{
   assert(self != NULL);
   adt_list_t const* list = &self->file_list;
   if (adt_list_is_empty(list))
   {
      return NULL;
   }
   adt_list_elem_t* next = adt_list_iter_first(list);
   adt_list_elem_t* candidate = NULL;
   //Find first candidate
   while (next != NULL)
   {
      apx_file_t* tmp = (apx_file_t*)next->pItem;
      assert(tmp != NULL);
      if (apx_file_get_apx_file_type(tmp) == file_type)
      {
         candidate = next;
         next = adt_list_iter_next(next);
         break;
      }
      next = adt_list_iter_next(next);
   }
   if (candidate != NULL)
   {
      //Find last candidate
      while (next != NULL)
      {
         apx_file_t* tmp = (apx_file_t*)next->pItem;
         assert(tmp != NULL);
         if (apx_file_get_apx_file_type(tmp) == file_type)
         {
            candidate = next;
            next = adt_list_iter_next(next);
         }
         else
         {
            break;
         }
      }
   }
   return candidate;
}

static adt_list_elem_t* find_next_available_position(apx_fileMap_t* self, apx_file_t* file)
{
   assert((self != NULL) && (file != NULL));
   adt_list_t const* list = &self->file_list;
   if (adt_list_is_empty(list))
   {
      return NULL;
   }
   adt_list_elem_t* iterator_next = adt_list_iter_first(list);
   adt_list_elem_t* iterator_prev = NULL;
   uint32_t const address = apx_file_get_address_without_flags(file);
   while (iterator_next != NULL)
   {
      apx_file_t* tmp = (apx_file_t*)iterator_next->pItem;
      assert(tmp != NULL);
      if (apx_file_get_address_without_flags(tmp) < address)
      {
         iterator_prev = iterator_next;
         iterator_next = adt_list_iter_next(iterator_next);
      }
      else
      {
         break;
      }
   }
   return iterator_prev;
}

static bool insert_item(apx_fileMap_t* self, apx_file_t* file, adt_list_elem_t* iterator_left)
{
   assert((self != NULL) && (file != NULL));
   adt_list_t* list = &self->file_list;
   if (adt_list_is_empty(list))
   {
      adt_list_insert(list, file);
   }
   else
   {
      uint32_t const file_start_address = apx_file_get_address_without_flags(file);
      uint32_t const file_end_address = apx_file_get_end_address_without_flags(file); // "End address" is first byte after valid address
      adt_list_elem_t* iterator_right = adt_list_iter_first(list);
      if (iterator_left != NULL)
      {
         apx_file_t* tmp = (apx_file_t*)iterator_left->pItem;
         assert(tmp != NULL);
         assert(apx_file_get_end_address_without_flags(tmp) <= file_start_address);
         iterator_right = adt_list_iter_next(iterator_left);
      }
      if (iterator_right == NULL)
      {
         adt_list_insert(list, file);
      }
      else
      {
         apx_file_t* right_file = (apx_file_t*)iterator_right->pItem;
         if ((file_end_address > apx_file_get_address_without_flags(right_file)) ||
            (file_end_address > RMF_CMD_AREA_START_ADDRESS))
         {
            return false; //This file would overlap with another file (or with the command area)
         }
         adt_list_insert_before(list, iterator_right, file);
      }
   }
   return true;
}