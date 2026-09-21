/*****************************************************************************
* \file      data_signature.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX parse tree: data signature
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_DATA_SIGNATURE_H
#define APX_DATA_SIGNATURE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/data_element.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_data_signature_tag
{
   apx_data_element_t *data_element; //strong reference
   apx_data_element_t *effective_data_element; //strong reference
   //TODO: Add support for client-server interfaces
} apx_data_signature_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_data_signature_create(apx_data_signature_t *self);
void apx_data_signature_destroy(apx_data_signature_t *self);
apx_data_element_t* apx_data_signature_get_data_element(apx_data_signature_t* self);
void apx_data_signature_set_element(apx_data_signature_t* self, apx_data_element_t* data_element);
apx_data_element_t* apx_data_signature_get_effective_data_element(apx_data_signature_t* self);
void apx_data_signature_set_effective_element(apx_data_signature_t* self, apx_data_element_t* data_element);
#endif //APX_DATA_SIGNATURE_H
