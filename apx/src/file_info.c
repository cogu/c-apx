/*****************************************************************************
* \file      apx_fileInfo.c
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Disposable file info data structure
*
* Copyright (c) 2020 Conny Gustafsson
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
#include "apx_fileInfo.h"
#include "bstr.h"
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
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileInfo_create(apx_fileInfo_t *self, uint32_t address, uint32_t length, const char *name, uint16_t fileType, uint16_t digestType, const uint8_t *digestData)
{
   if ( (self != 0) && (name != 0) )
   {
      apx_fileInfo_setAddress(self, address);
      self->length = length;
      self->fileType = fileType;
      self->digestType = digestType;
      self->name = STRDUP(name);

      if (self->name == 0)
      {
         return APX_MEM_ERROR;
      }
      if (digestData != 0)
      {
         self->digestData = (uint8_t*) malloc(RMF_DIGEST_SIZE);
         if (self->digestData == 0)
         {
            return APX_MEM_ERROR;
         }
         memcpy(self->digestData, digestData, RMF_DIGEST_SIZE);
      }
      else
      {
         self->digestData = (uint8_t*) 0;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileInfo_create_rmf(apx_fileInfo_t *self, const rmf_fileInfo_t *fileInfo, bool isRemoteAddress)
{
   if (fileInfo != 0)
   {
      uint32_t address;
      const uint8_t *digestData = (const uint8_t*) 0;
      uint32_t addressFlag = isRemoteAddress? RMF_REMOTE_ADDRESS_BIT : 0u;
      address  = (fileInfo->address & RMF_ADDRESS_MASK) | addressFlag;
      if (fileInfo->digestType != RMF_DIGEST_TYPE_NONE)
      {
         digestData = &fileInfo->digestData[0];
      }
      return apx_fileInfo_create(self, address, fileInfo->length, &fileInfo->name[0], fileInfo->fileType, fileInfo->digestType, digestData);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileInfo_destroy(apx_fileInfo_t *self)
{
   if (self != 0)
   {
      if (self->name != 0)
      {
         free(self->name);
      }
      if (self->digestData != 0)
      {
         free(self->digestData);
      }
   }
}

apx_fileInfo_t* apx_fileInfo_new(uint32_t address, uint32_t length, const char *name, uint16_t fileType, uint16_t digestType, const uint8_t *digestData)
{
   apx_fileInfo_t *self = (apx_fileInfo_t*) malloc(sizeof(apx_fileInfo_t));
   if (self != 0)
   {
      apx_error_t rc = apx_fileInfo_create(self, address, length, name, fileType, digestType, digestData);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = (apx_fileInfo_t*) 0;
      }
   }
   return self;
}

apx_fileInfo_t* apx_fileInfo_new_rmf(rmf_fileInfo_t *fileInfo, bool isRemoteAddress)
{
   if (fileInfo != 0)
   {
      apx_fileInfo_t *self = (apx_fileInfo_t*) malloc(sizeof(apx_fileInfo_t));
      if (self != 0)
      {
         apx_error_t rc = apx_fileInfo_create_rmf(self, fileInfo, isRemoteAddress);
         if (rc != APX_NO_ERROR)
         {
            free(self);
            self = (apx_fileInfo_t*) 0;
         }
      }
      return self;
   }
   return (apx_fileInfo_t*) 0;
}

apx_error_t apx_fileInfo_assign(apx_fileInfo_t *self, const apx_fileInfo_t *other)
{
   if ( (self != 0) && (other != 0) )
   {
      apx_fileInfo_setAddress(self, other->address);
      self->length = other->length;
      self->fileType = other->fileType;
      self->digestType = other->digestType;
      self->name = (char*) 0;
      self->digestData = (uint8_t*) 0;
      if (other->name != 0)
      {
         self->name = STRDUP(other->name);
         if (self->name == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      if(other->digestData != 0)
      {
         self->digestData = (uint8_t*) malloc(RMF_DIGEST_SIZE);
         if (self->digestData == 0)
         {
            return APX_MEM_ERROR;
         }
         memcpy(self->digestData, other->digestData, RMF_DIGEST_SIZE);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_fileInfo_t* apx_fileInfo_clone(const apx_fileInfo_t *other)
{
   if (other != 0)
   {
      apx_fileInfo_t *self = (apx_fileInfo_t*) malloc(sizeof(apx_fileInfo_t));
      if (self != 0)
      {
         apx_error_t rc = apx_fileInfo_create(self, other->address, other->length, other->name, other->fileType, other->digestType, other->digestData);
         if (rc != APX_NO_ERROR)
         {
            free(self);
            self = (apx_fileInfo_t*) 0;
         }
      }
      return self;
   }
   return (apx_fileInfo_t*) 0;
}

void apx_fileInfo_setAddress(apx_fileInfo_t *self, uint32_t address)
{
   self->address = address;
   self->addressWithoutFlags = address & RMF_ADDRESS_MASK_INTERNAL;
}

void apx_fileInfo_delete(apx_fileInfo_t *self)
{
   if (self != 0)
   {
      apx_fileInfo_destroy(self);
      free(self);
   }
}

void apx_fileInfo_vdelete(void *arg)
{
   apx_fileInfo_delete((apx_fileInfo_t*) arg);
}

void apx_fileInfo_fillRmfInfo(const apx_fileInfo_t *self, rmf_fileInfo_t *rmfInfo)
{
   if ( (self != 0) && (rmfInfo != 0) )
   {
      rmfInfo->address = self->address;
      rmfInfo->fileType = self->fileType;
      rmfInfo->length = self->length;
      rmfInfo->digestType = self->digestType;
      if (self->name != 0)
      {
         strcpy(&rmfInfo->name[0], self->name);
      }
      else
      {
         memset(&rmfInfo->name[0], 0, RMF_MAX_FILE_NAME+1);
      }
      if (self->digestData != 0)
      {
         memcpy(&rmfInfo->digestData[0], self->digestData, RMF_DIGEST_SIZE);
      }
      else
      {
         memset(&rmfInfo->digestData[0], 0, RMF_DIGEST_SIZE);
      }
   }
}

bool apx_fileInfo_isRemoteAddress(apx_fileInfo_t *self)
{
   if (self != 0)
   {
      return ( (self->address != RMF_INVALID_ADDRESS) &&  ( (self->address & RMF_REMOTE_ADDRESS_BIT) != 0u) );
   }
   return false;
}


bool apx_fileInfo_nameEndsWith(const apx_fileInfo_t *self, const char* suffix)
{
   if ( (self != 0) && (suffix != 0) && (self->name != 0) )
   {
      size_t str_len = strlen(self->name);
      size_t suffix_len = strlen(suffix);
      return (str_len >= suffix_len) && (strcmp(self->name + (str_len-suffix_len), suffix) == 0);
   }
   return false;
}

char *apx_fileInfo_getBaseName(const apx_fileInfo_t *self)
{
   if ( (self != 0) && (self->name != 0))
   {
      char *dot = strchr(self->name, '.');
      if (dot != 0)
      {
         return bstr_make_cstr((const uint8_t*) self->name, (const uint8_t*) dot);
      }
   }
   return (char*) 0;
}

void apx_fileInfo_copyBaseName(const apx_fileInfo_t *self, char *dest, uint32_t maxDestLen)
{
   if ( (self != 0) && (self->name != 0) && (maxDestLen > 1))
   {
      char *dot = strchr(self->name, '.');
      if (dot != 0)
      {
         //uint32_t len = (uint32_t) strlen(dot);
         uint32_t len = (uint32_t) (dot - self->name);
         if (maxDestLen < len)
         {
            strncpy(dest, self->name, maxDestLen-1);
            dest[maxDestLen-1] = '\0';
         }
         else
         {
            strncpy(dest, self->name, len);
            dest[len] = '\0';
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


