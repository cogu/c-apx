/*****************************************************************************
* \file      parser.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX parser
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PARSER_H
#define APX_PARSER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "apx/error.h"
#include "apx/stream.h"
#include "apx/node.h"
#include "apx/data_element.h"
#include "apx/port_attribute.h"
#include "apx/type_attribute.h"
#include "apx/attribute_parser.h"
#include "apx/signature_parser.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef uint8_t apx_definition_section_t;
#define APX_DEFINITION_SECTION_VERSION ((apx_definition_section_t) 0u)
#define APX_DEFINITION_SECTION_NODE    ((apx_definition_section_t) 1u)
#define APX_DEFINITION_SECTION_TYPE    ((apx_definition_section_t) 2u)
#define APX_DEFINITION_SECTION_PORT    ((apx_definition_section_t) 3u)

typedef struct apx_parse_state_tag
{
   apx_definition_section_t accept_next;
   int32_t major_version;
   int32_t minor_version;
   uint32_t lineno;
   apx_node_t* node; //Strong reference
   apx_data_element_t* data_element; //Weak reference
   apx_data_type_t* data_type; //Weak reference
   apx_port_t* port; //Weak reference
} apx_parse_state_t;

typedef struct apx_parser_tag
{
   apx_istream_t* stream;
   apx_error_t last_error;
   int32_t last_error_line;
   apx_parse_state_t state;
   apx_attribute_parser_t attribute_parser;
   apx_signature_parser_t signature_parser;
} apx_parser_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_parser_create(apx_parser_t *self, apx_istream_t *stream);
void apx_parser_destroy(apx_parser_t *self);
apx_parser_t* apx_parser_new(apx_istream_t* stream);
void apx_parser_delete(apx_parser_t *self);
apx_node_t *apx_parser_take_last_node(apx_parser_t *self);
apx_error_t apx_parser_get_last_error(apx_parser_t *self);
int32_t apx_parser_get_error_line(apx_parser_t *self);
apx_error_t apx_parser_parse_cstr(apx_parser_t *self, const char *apx_text);
apx_error_t apx_parser_parse_bstr(apx_parser_t* self, uint8_t const* begin, uint8_t const* end);


#endif //APX_PARSER_H
