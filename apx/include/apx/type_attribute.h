/*****************************************************************************
* \file      type_attribute.h
* \author    Conny Gustafsson
* \date      2018-09-11
* \brief     APX type attributes
*
* Copyright (c) 2018 Conny Gustafsson
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
