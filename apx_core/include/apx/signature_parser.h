/*****************************************************************************
* \file      signature_parser.h
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     Data signature parser
*
* Copyright (c) 2020 Conny Gustafsson
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
#ifndef APX_SIGNATURE_PARSER_H
#define APX_SIGNATURE_PARSER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/data_element.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_signatureParserState_tag
{
   bool is_record;
   apx_dataElement_t *data_element; //strong reference
} apx_signatureParserState_t;

typedef struct apx_signatureParser_tag
{
   apx_signatureParserState_t *state;
   apx_error_t error_code;
   uint8_t const* error_pos;
} apx_signatureParser_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_signatureParserState_create(apx_signatureParserState_t* self, bool is_record);
void apx_signatureParserState_destroy(apx_signatureParserState_t* self);
apx_signatureParserState_t* apx_signatureParserState_new(bool is_record);
void apx_signatureParserState_delete(apx_signatureParserState_t* self);

apx_error_t apx_signatureParser_create(apx_signatureParser_t* self);
void apx_signatureParser_destroy(apx_signatureParser_t* self);
uint8_t const* apx_signatureParser_parse_data_signature(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
apx_error_t apx_signatureParser_get_last_error(apx_signatureParser_t* self, uint8_t const** error_pos);
apx_dataElement_t* apx_signatureParser_take_data_element(apx_signatureParser_t* self);


#endif //APX_SIGNATURE_PARSER_H
