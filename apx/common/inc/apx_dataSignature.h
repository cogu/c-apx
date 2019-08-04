/*****************************************************************************
* \file      apx_dataSignature.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Data Signature (DSG) container and parser logic
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
#ifndef APX_DATASIGNATURE_H
#define APX_DATASIGNATURE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "apx_dataElement.h"
#include "apx_error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct adt_ary_tag;
struct adt_hash_tag;
struct adt_str_tag;

typedef struct apx_dataSignature_tag
{
   char *raw;
   char *derived;
   uint8_t dsgType; //this will always have value APX_DSG_TYPE_SENDER_RECEIVER until client/server has been implemented
   apx_dataElement_t *dataElement;
   apx_error_t lastError;
   //TODO: implement support for client/server interfaces here
}apx_dataSignature_t;

#define APX_DSG_TYPE_SENDER_RECEIVER   0
#define APX_DSG_TYPE_CLIENT_SERVER     1


//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_dataSignature_t *apx_dataSignature_new(const char *dsg, apx_error_t *errorCode);
void apx_dataSignature_delete(apx_dataSignature_t *self);
void apx_dataSignature_vdelete(void *arg);
apx_error_t apx_dataSignature_create(apx_dataSignature_t *self, const char *dsg);
void apx_dataSignature_destroy(apx_dataSignature_t *self);
int32_t apx_dataSignature_getPackLen(apx_dataSignature_t *self);
int32_t apx_dataSignature_calcPackLen(apx_dataSignature_t *self);
apx_error_t apx_dataSignature_resolveTypes(apx_dataSignature_t *self, struct adt_ary_tag *typeList, struct adt_hash_tag *typeMap);
const char *apx_dataSignature_getDerivedString(apx_dataSignature_t *self);
apx_dataElement_t *apx_dataSignature_getDerivedDataElement(apx_dataSignature_t *self);
apx_error_t apx_dataSignature_getLastError(apx_dataSignature_t *self);

#endif //APX_DATASIGNATURE_H
