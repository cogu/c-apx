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

typedef struct apx_typeAttributes_tag
{
   adt_ary_t computations;
} apx_typeAttributes_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_typeAttributes_create(apx_typeAttributes_t *self);
void apx_typeAttributes_destroy(apx_typeAttributes_t *self);
apx_typeAttributes_t *apx_typeAttributes_new(void);
void apx_typeAttributes_delete(apx_typeAttributes_t *self);

void apx_typeAttributes_append_computation(apx_typeAttributes_t* self, struct apx_computation_tag* computation);
int32_t apx_typeAttributes_num_computations(apx_typeAttributes_t* self);
struct apx_computation_tag* apx_typeAttributes_get_computation(apx_typeAttributes_t* self, int32_t index);

#endif //APX_TYPE_ATTRIBUTE_H
