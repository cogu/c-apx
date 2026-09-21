/*****************************************************************************
* \file      data_type.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX datatype class
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
struct apx_data_element_tag;

typedef struct apx_data_type_tag
{
   char *name;
   apx_type_attributes_t *attributes;
   apx_data_signature_t data_signature;
   apx_type_id_t type_id;
   int32_t line_number;
} apx_data_type_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
apx_data_type_t* apx_dataType_new(const char *name, int32_t line_number);
void apx_dataType_delete(apx_data_type_t*self);
void apx_dataType_vdelete(void *arg);
apx_error_t apx_dataType_create(apx_data_type_t*self, const char *name, int32_t line_number);
void apx_dataType_destroy(apx_data_type_t*self);
apx_data_element_t* apx_dataType_get_data_element(apx_data_type_t* self);
int32_t apx_dataType_get_line_number(apx_data_type_t*self);
bool apx_dataType_has_attributes(apx_data_type_t* self);
apx_error_t apx_dataType_init_attributes(apx_data_type_t* self);
apx_type_attributes_t* apx_dataType_get_attributes(apx_data_type_t* self);
const char* apx_dataType_get_name(apx_data_type_t* self);
void apx_dataType_set_id(apx_data_type_t* self, apx_type_id_t type_id);
apx_type_id_t apx_dataType_get_id(apx_data_type_t const* self);
apx_error_t apx_dataType_derive_types_on_element(apx_data_type_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map);
apx_error_t apx_dataType_derive_data_element(apx_data_type_t* self, struct apx_data_element_tag** data_element, struct apx_data_element_tag** parent);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_DATATYPE_H
