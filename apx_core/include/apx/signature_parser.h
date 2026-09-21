/*****************************************************************************
* \file      signature_parser.h
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     Data signature parser
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
typedef struct apx_signature_parser_state_tag
{
   bool is_record;
   apx_data_element_t *data_element; //strong reference
} apx_signature_parser_state_t;

typedef struct apx_signature_parser_tag
{
   apx_signature_parser_state_t *state;
   apx_error_t error_code;
   uint8_t const* error_pos;
} apx_signature_parser_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_signatureParserState_create(apx_signature_parser_state_t* self, bool is_record);
void apx_signatureParserState_destroy(apx_signature_parser_state_t* self);
apx_signature_parser_state_t* apx_signatureParserState_new(bool is_record);
void apx_signatureParserState_delete(apx_signature_parser_state_t* self);

apx_error_t apx_signatureParser_create(apx_signature_parser_t* self);
void apx_signatureParser_destroy(apx_signature_parser_t* self);
uint8_t const* apx_signatureParser_parse_data_signature(apx_signature_parser_t* self, uint8_t const* begin, uint8_t const* end);
apx_error_t apx_signatureParser_get_last_error(apx_signature_parser_t* self, uint8_t const** error_pos);
apx_data_element_t* apx_signatureParser_take_data_element(apx_signature_parser_t* self);


#endif //APX_SIGNATURE_PARSER_H
