/*****************************************************************************
* \file      type_attribute.h
* \author    Conny Gustafsson
* \date      2018-09-11
* \brief     APX type attributes
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_TYPE_ATTRIBUTE_H
#define APX_TYPE_ATTRIBUTE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "adt_ary.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//Forward declarations
struct apx_computation_tag;

typedef struct apx_type_attributes_tag
{
   adt_ary_t computations;
} apx_type_attributes_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_type_attributes_create(apx_type_attributes_t *self);
void apx_type_attributes_destroy(apx_type_attributes_t *self);
apx_type_attributes_t *apx_type_attributes_new(void);
void apx_type_attributes_delete(apx_type_attributes_t *self);

void apx_type_attributes_append_computation(apx_type_attributes_t* self, struct apx_computation_tag* computation);
int32_t apx_type_attributes_num_computations(apx_type_attributes_t* self);
struct apx_computation_tag* apx_type_attributes_get_computation(apx_type_attributes_t* self, int32_t index);

#endif //APX_TYPE_ATTRIBUTE_H
