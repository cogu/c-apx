/*****************************************************************************
* \file      apx_datatype.h
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
#ifndef APX_DATATYPE_H
#define APX_DATATYPE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "apx_dataSignature.h"
#include "apx_typeAttribute.h"
#include "apx_error.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_datatype_tag
{
   char *name;
   apx_typeAttribute_t *attribute;
   apx_dataSignature_t *dataSignature;
   int32_t lineNumber;
   apx_error_t lastError;
}apx_datatype_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
apx_datatype_t* apx_datatype_new(const char *name, const char *dsg, const char *attr, int32_t lineNumber, apx_error_t *errorCode);
void apx_datatype_delete(apx_datatype_t *self);
void apx_datatype_vdelete(void *arg);
apx_error_t apx_datatype_create(apx_datatype_t *self, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
void apx_datatype_destroy(apx_datatype_t *self);
int32_t apx_datatype_getLineNumber(apx_datatype_t *self);
apx_error_t apx_datatype_calcPackLen(apx_datatype_t *self, apx_size_t *packLen);
apx_error_t apx_datatype_getLastError(apx_datatype_t *self);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_DATATYPE_H
