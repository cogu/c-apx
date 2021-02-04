/*****************************************************************************
* \file      data_signature.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX parse tree: data signature
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
