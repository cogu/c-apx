/*****************************************************************************
* \file      apx_file.c
* \author    Conny Gustafsson
* \date      2018-08-30
* \brief     Improved version of apx_file
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
#include <string.h>
#include <errno.h>
#include "apx/error.h"
#include "apx/file.h"
#include "bstr.h"
#include "apx/event_listener.h"
#include "apx/node_data.h"
#include "apx/file_manager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void apx_file_calc_file_type(apx_file_t *self);
static void apx_file_lock(apx_file_t *self);
static void apx_file_unlock(apx_file_t *self);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_file_create(apx_file_t* self, const rmf_fileInfo_t* file_info)
{
   if ( (self != NULL) && (file_info != NULL))
   {
      apx_error_t retval = APX_NO_ERROR;
      self->is_file_open = false;
      self->has_first_write = false;
      self->file_manager = (apx_fileManager_t*) NULL;
      self->apx_file_type = APX_UNKNOWN_FILE_TYPE;
      memset(&self->notification_handler, 0, sizeof(apx_fileNotificationHandler_t));      
      retval = rmf_fileInfo_create_copy(&self->file_info, file_info);
      if (retval == APX_NO_ERROR)
      {
         apx_file_calc_file_type(self);
         MUTEX_INIT(self->lock);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_destroy(apx_file_t *self)
{
   if (self != 0)
   {
      rmf_fileInfo_destroy(&self->file_info);      
      MUTEX_DESTROY(self->lock);
   }
}

apx_file_t* apx_file_new(const rmf_fileInfo_t* file_info)
{
   apx_file_t *self = (apx_file_t*) malloc(sizeof(apx_file_t));
   if (self != 0)
   {
      apx_error_t result = apx_file_create(self, file_info);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = (apx_file_t*) 0;
      }
   }
   return self;
}

void apx_file_delete(apx_file_t *self)
{
   if (self != 0)
   {
      apx_file_destroy(self);
      free(self);
   }
}

void apx_file_vdelete(void *arg)
{
   apx_file_delete((apx_file_t*) arg);
}

void apx_file_open(apx_file_t *self)
{
   if (self != 0)
   {
      apx_file_lock(self);
      self->is_file_open = true;
      apx_file_unlock(self);
   }
}

void apx_file_close(apx_file_t *self)
{
   if (self != 0)
   {
      apx_file_lock(self);
      self->is_file_open = false;
      apx_file_unlock(self);
   }
}

void apx_file_set_notification_handler(apx_file_t* self, const apx_fileNotificationHandler_t* handler)
{
   if (self != 0)
   {
      apx_file_lock(self);
      if (handler == 0)
      {
         memset(&self->notification_handler, 0, sizeof(apx_fileNotificationHandler_t));
      }
      else
      {
         memcpy(&self->notification_handler, handler, sizeof(apx_fileNotificationHandler_t));
      }
      apx_file_unlock(self);
   }
}

bool apx_file_has_first_write(apx_file_t* self)
{
   if (self != 0)
   {
      return self->has_first_write;
   }
   return false;
}

void apx_file_set_first_write(apx_file_t* self)
{
   if (self != 0)
   {
      self->has_first_write = true;
   }
}


bool apx_file_is_open(apx_file_t* self)
{
   if (self != 0)
   {
      bool retval;
      apx_file_lock(self);
      retval = self->is_file_open;
      apx_file_unlock(self);
      return retval;
   }
   return false;
}

bool apx_file_is_local(apx_file_t *self)
{
   if (self != 0)
   {
      uint32_t const address = rmf_fileInfo_address(&self->file_info);
      uint32_t const address_without_flags = address & APX_ADDRESS_MASK_INTERNAL;
      return (address_without_flags != RMF_INVALID_ADDRESS) && ((address & RMF_REMOTE_ADDRESS_BIT) == 0u);
   }
   return false;
}

bool apx_file_is_remote(apx_file_t *self)
{
   if (self != 0)
   {
      uint32_t const address = rmf_fileInfo_address(&self->file_info);
      uint32_t const address_without_flags = address & APX_ADDRESS_MASK_INTERNAL;
      return (address_without_flags != RMF_INVALID_ADDRESS) && ((address & RMF_REMOTE_ADDRESS_BIT) != 0u);
   }
   return false;
}

bool apx_file_has_valid_address(apx_file_t* self)
{
   if (self != 0)
   {
      uint32_t const address_without_flags = rmf_fileInfo_address_without_flags(&self->file_info);
      return (address_without_flags != RMF_INVALID_ADDRESS);
   }
   return false;
}

struct apx_fileManager_tag* apx_file_get_file_manager(apx_file_t* self)
{
   if (self != 0)
   {
      return self->file_manager;
   }
   return (apx_fileManager_t*) 0;
}

void apx_file_set_file_manager(apx_file_t* self, struct apx_fileManager_tag* file_manager)
{
   if (self != 0)
   {
      self->file_manager = file_manager;
   }
}

apx_fileType_t apx_file_get_apx_file_type(const apx_file_t* self)
{
   if (self != NULL)
   {
      return self->apx_file_type;
   }
   return APX_UNKNOWN_FILE_TYPE;
}

uint32_t apx_file_get_address(const apx_file_t *self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_address(&self->file_info);
   }
   return RMF_INVALID_ADDRESS;
}

uint32_t apx_file_get_address_without_flags(const apx_file_t* self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_address_without_flags(&self->file_info);
   }
   return RMF_INVALID_ADDRESS;
}

void apx_file_set_address(apx_file_t* self, uint32_t address)
{
   if (self != NULL)
   {
      rmf_fileInfo_set_address(&self->file_info, address);
   }
}

uint32_t apx_file_get_size(const apx_file_t* self)
{
   if (self != NULL)
   {
      rmf_fileInfo_size(&self->file_info);
   }
   return 0;
}

const char* apx_file_get_name(const apx_file_t* self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_name(&self->file_info);
   }
   return NULL;
}

uint32_t apx_file_get_end_address(const apx_file_t* self)
{
   if (self != NULL)
   {
      return self->file_info.address + self->file_info.size;
   }
   return RMF_INVALID_ADDRESS;
}

uint32_t apx_file_get_end_address_without_flags(const apx_file_t* self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_address_without_flags(&self->file_info) + self->file_info.size;
   }
   return RMF_INVALID_ADDRESS;
}

bool apx_file_address_in_range(const apx_file_t* self, uint32_t address)
{
   if (self != NULL)
   {
      return rmf_fileInfo_address_in_range(&self->file_info, address);
   }
   return false;
}

rmf_fileInfo_t const* apx_file_get_file_info(const apx_file_t* self)
{
   if (self != NULL)
   {
      return &self->file_info;
   }
   return NULL;
}

rmf_digestType_t apx_file_get_digest_type(apx_file_t const* self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_digest_type(&self->file_info);
   }
   return RMF_DIGEST_TYPE_NONE;
}

uint8_t const* apx_file_get_digest_data(const apx_file_t* self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_digest_data(&self->file_info);
   }
   return NULL;
}

rmf_fileInfo_t* apx_file_clone_file_info(const apx_file_t* self)
{
   if (self != NULL)
   {
      return rmf_fileInfo_clone(&self->file_info);
   }
   return NULL;
}

apx_error_t apx_file_open_notify(apx_file_t* self)
{
   if (self != NULL)
   {
      if (self->notification_handler.open_notify != NULL)
      {
         return self->notification_handler.open_notify(self->notification_handler.arg, self);
      }
      return APX_INVALID_OPEN_HANDLER_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_write_notify(apx_file_t* self, uint32_t offset, const uint8_t* src, uint32_t len)
{
   if (self != 0)
   {
      apx_file_lock(self);
      if (!self->has_first_write)
      {
         self->has_first_write = true;
      }
      apx_file_unlock(self);
      if (self->notification_handler.write_notify != 0)
      {
         return self->notification_handler.write_notify(self->notification_handler.arg, self, offset, src, len);
      }
      return APX_INVALID_WRITE_HANDLER_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//global functions
char const* apx_file_type_to_extension(apx_fileType_t file_type)
{
   switch (file_type)
   {
   case APX_DEFINITION_FILE_TYPE:
      return APX_DEFINITION_FILE_EXT;
   case APX_PROVIDE_PORT_DATA_FILE_TYPE:
      return APX_PROVIDE_PORT_DATA_EXT;
   case APX_REQUIRE_PORT_DATA_FILE_TYPE:
      return APX_REQUIRE_PORT_DATA_EXT;
   case APX_PROVIDE_PORT_COUNT_FILE_TYPE:
      return APX_PROVIDE_PORT_COUNT_EXT;
   case APX_REQUIRE_PORT_COUNT_FILE_TYPE:
      return APX_REQUIRE_PORT_COUNT_EXT;
   }
   return "";
}

bool apx_file_less_than(const apx_file_t* a, const apx_file_t* b)
{
   if ((a != NULL) && (b != NULL))
   {
      return apx_file_get_address_without_flags(a) < apx_file_get_address_without_flags(b);
   }
   return false;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_file_calc_file_type(apx_file_t *self)
{
   if (rmf_fileInfo_name_ends_with(&self->file_info, APX_DEFINITION_FILE_EXT))
   {
      self->apx_file_type = APX_DEFINITION_FILE_TYPE;
   }
   else if (rmf_fileInfo_name_ends_with(&self->file_info, APX_PROVIDE_PORT_DATA_EXT))
   {
      self->apx_file_type = APX_PROVIDE_PORT_DATA_FILE_TYPE;
   }
   else if (rmf_fileInfo_name_ends_with(&self->file_info, APX_REQUIRE_PORT_DATA_EXT))
   {
      self->apx_file_type = APX_REQUIRE_PORT_DATA_FILE_TYPE;
   }
   else if (rmf_fileInfo_name_ends_with(&self->file_info, APX_PROVIDE_PORT_COUNT_EXT))
   {
      self->apx_file_type = APX_PROVIDE_PORT_COUNT_FILE_TYPE;
   }
   else if (rmf_fileInfo_name_ends_with(&self->file_info, APX_REQUIRE_PORT_COUNT_EXT))
   {
      self->apx_file_type = APX_REQUIRE_PORT_COUNT_FILE_TYPE;
   }
   else
   {
      self->apx_file_type = APX_UNKNOWN_FILE_TYPE;
   }
}

static void apx_file_lock(apx_file_t *self)
{
   if(self != 0)
   {
      MUTEX_LOCK(self->lock);
   }
}

static void apx_file_unlock(apx_file_t *self)
{
   if(self != 0)
   {
      MUTEX_UNLOCK(self->lock);
   }
}

