/*****************************************************************************
* \file      type_attribute.c
* \author    Conny Gustafsson
* \date      2018-09-11
* \brief     Parse tree APX type attribute
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
#include "apx/type_attribute.h"
#include "apx/types.h"
#include <string.h>
#include <malloc.h>
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
apx_error_t apx_typeAttribute_create(apx_typeAttribute_t *self, const char *attr)
{
   if (self != 0)
   {
      self->rawString = (attr!=0)? STRDUP(attr) : 0;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_typeAttribute_destroy(apx_typeAttribute_t *self)
{
   if (self != 0)
   {
      if (self->rawString != 0)
      {
         free(self->rawString);
      }
   }
}

apx_typeAttribute_t *apx_typeAttribute_new(const char *attr)
{
   apx_typeAttribute_t *self = (apx_typeAttribute_t*) malloc(sizeof(apx_typeAttribute_t));
   if (self != 0)
   {
      apx_error_t errorCode = apx_typeAttribute_create(self, attr);
      if (errorCode != APX_NO_ERROR)
      {
         free(self);
         self = (apx_typeAttribute_t*) 0;
      }
   }
   return self;
}

void apx_typeAttribute_delete(apx_typeAttribute_t *self)
{
   if (self != 0)
   {
      apx_typeAttribute_destroy(self);
      free(self);
   }
}

const char *apx_typeAttribute_cstr(apx_typeAttribute_t *self)
{
   if (self != 0)
   {
      return self->rawString;
   }
   return (const char*) 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


