/*****************************************************************************
* \file      parser.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX parser
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
   apx_dataElement_t* data_element; //Weak reference
   apx_dataType_t* data_type; //Weak reference
   apx_port_t* port; //Weak reference
} apx_parse_state_t;

typedef struct apx_parser_tag
{
   apx_istream_t* stream;
   apx_error_t last_error;
   int32_t last_error_line;
   apx_parse_state_t state;
   apx_attributeParser_t attribute_parser;
   apx_signatureParser_t signature_parser;
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
