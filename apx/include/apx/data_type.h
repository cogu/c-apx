/*****************************************************************************
* \file      datatype.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX datatype class
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include "apx/data_signature.h"
#include "apx/type_attribute.h"
#include "apx/error.h"
#include "adt_ary.h"
#include "adt_hash.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_dataElement_tag;

typedef struct apx_dataType_tag
{
   char *name;
   apx_typeAttributes_t *attributes;
   apx_dataSignature_t data_signature;
   apx_typeId_t type_id;
   int32_t line_number;
} apx_dataType_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
apx_dataType_t* apx_dataType_new(const char *name, int32_t line_number);
void apx_dataType_delete(apx_dataType_t*self);
void apx_dataType_vdelete(void *arg);
apx_error_t apx_dataType_create(apx_dataType_t*self, const char *name, int32_t line_number);
void apx_dataType_destroy(apx_dataType_t*self);
apx_dataElement_t* apx_dataType_get_data_element(apx_dataType_t* self);
int32_t apx_dataType_get_line_number(apx_dataType_t*self);
bool apx_dataType_has_attributes(apx_dataType_t* self);
apx_error_t apx_dataType_init_attributes(apx_dataType_t* self);
apx_typeAttributes_t* apx_dataType_get_attributes(apx_dataType_t* self);
const char* apx_dataType_get_name(apx_dataType_t* self);
void apx_dataType_set_id(apx_dataType_t* self, apx_typeId_t type_id);
apx_typeId_t apx_dataType_get_id(apx_dataType_t const* self);
apx_error_t apx_dataType_derive_types_on_element(apx_dataType_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map);
apx_error_t apx_dataType_derive_data_element(apx_dataType_t* self, struct apx_dataElement_tag** data_element, struct apx_dataElement_tag** parent);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_DATATYPE_H
