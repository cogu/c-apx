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

typedef struct apx_dataSignature_tag
{
   apx_dataElement_t *data_element; //strong reference
   apx_dataElement_t *effective_data_element; //strong reference
   //TODO: Add support for client-server interfaces
} apx_dataSignature_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_dataSignature_create(apx_dataSignature_t *self);
void apx_dataSignature_destroy(apx_dataSignature_t *self);
apx_dataElement_t* apx_dataSignature_get_data_element(apx_dataSignature_t* self);
void apx_dataSignature_set_element(apx_dataSignature_t* self, apx_dataElement_t* data_element);
apx_dataElement_t* apx_dataSignature_get_effective_data_element(apx_dataSignature_t* self);
void apx_dataSignature_set_effective_element(apx_dataSignature_t* self, apx_dataElement_t* data_element);
#endif //APX_DATA_SIGNATURE_H
