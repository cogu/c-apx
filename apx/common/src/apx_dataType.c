/*****************************************************************************
* \file      apx_datatype.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX datatype class
*
* Copyright (c) 2017-2018 Conny Gustafsson
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>
#include "apx_dataType.h"
#include "apx_types.h"
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
apx_datatype_t* apx_datatype_new(const char *name, const char *dsg, const char *attr, int32_t lineNumber, apx_error_t *errorCode)
{
   apx_datatype_t *self = (apx_datatype_t*) malloc(sizeof(apx_datatype_t));
   if(self != 0)
   {
      apx_error_t result = apx_datatype_create(self, name, dsg, attr, lineNumber);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self=0;
      }
      if (errorCode != 0)
      {
         *errorCode = result;
      }
   }
   else
   {
      if (errorCode != 0)
      {
         *errorCode = APX_MEM_ERROR;
      }
   }
   return self;
}

void apx_datatype_delete(apx_datatype_t *self)
{
   if(self != 0){
      apx_datatype_destroy(self);
      free(self);
   }
}

void apx_datatype_vdelete(void *arg)
{
   apx_datatype_delete((apx_datatype_t*) arg);
}

apx_error_t apx_datatype_create(apx_datatype_t *self, const char *name, const char *dsg, const char *attr, int32_t lineNumber)
{
   if (self != 0)
   {
      if (name != 0)
      {
         self->name = STRDUP(name);
         if (self->name == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      else
      {
         self->name = 0;
      }
      if (dsg != 0)
      {
         apx_error_t err;
         self->dataSignature = apx_dataSignature_new(dsg, &err);
         if (self->dataSignature == 0)
         {
            if (self->name != NULL)
            {
               free(self->name);
            }
            return err;
         }
      }
      else
      {
         self->dataSignature = NULL;
      }

      if (attr != 0)
      {
         self->attribute = apx_typeAttribute_new(attr);
         if (self->attribute == 0)
         {
            if (self->name != NULL)
            {
               free(self->name);
            }
            if (self->dataSignature != 0)
            {
               apx_dataSignature_delete(self->dataSignature);
            }
            return APX_MEM_ERROR;
         }
      }
      else
      {
         self->attribute = NULL;
      }
      self->lineNumber = lineNumber;
   }
   return APX_NO_ERROR;
}

void apx_datatype_destroy(apx_datatype_t *self)
{
   if ( self !=0 )
   {
      if (self->name != 0)
      {
         free(self->name);
         self->name = NULL;
      }
      if (self->dataSignature != 0)
      {
         apx_dataSignature_delete(self->dataSignature);
         self->dataSignature = NULL;
      }
      if (self->attribute != 0)
      {
         apx_typeAttribute_delete(self->attribute);
         self->attribute = NULL;
      }
   }
}

int32_t apx_datatype_getLineNumber(apx_datatype_t *self)
{
   if (self != 0)
   {
      return self->lineNumber;
   }
   return -1;
}

apx_error_t apx_datatype_calcPackLen(apx_datatype_t *self, apx_size_t *packLen)
{
   if ( (self != 0) && (packLen != 0))
   {
      apx_error_t result = APX_NO_ERROR;
      if (self->dataSignature != 0)
      {
         result = apx_dataSignature_calcPackLen(self->dataSignature, packLen);
      }
      else
      {
         return APX_DATA_SIGNATURE_ERROR;
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_datatype_getLastError(apx_datatype_t *self)
{
   if ( (self != 0) && (self->dataSignature != 0) )
   {
      return apx_dataSignature_getLastError(self->dataSignature);
   }
   return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


