/*****************************************************************************
* \file      attribute_parser.h
* \author    Conny Gustafsson
* \date      2017-07-30
* \brief     Port attribute parser
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
#ifndef APX_ATTRIBUTE_PARSER_H
#define APX_ATTRIBUTE_PARSER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_stack.h"
#include "apx/types.h"
#include "apx/port_attributes.h"
#include "apx/error.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_attributeParser_tag
{
   apx_error_t lastError;
   const uint8_t *pErrorNext;
   int32_t lastErrorPos;
   int16_t majorVersion;
   int16_t minorVersion;
}apx_attributeParser_t;

#ifdef UNIT_TEST
#define DYN_STATIC
#else
#define DYN_STATIC static
#endif

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_attributeParser_create(apx_attributeParser_t *self);
void apx_attributeParser_destroy(apx_attributeParser_t *self);
void apx_attributeParser_setVersion(apx_attributeParser_t *self, int16_t majorVersion, int16_t minorVersion);
apx_error_t apx_attributeParser_parseObject(apx_attributeParser_t *self, apx_portAttributes_t *attributeObject);
const uint8_t* apx_attributeParser_parse(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr);
apx_error_t apx_attributeParser_getLastError(apx_attributeParser_t *self, const uint8_t **ppNext);

#ifdef UNIT_TEST
DYN_STATIC const uint8_t* apx_attributeParser_parseSingleAttribute(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr);
DYN_STATIC const uint8_t* apx_attributeParser_parseInitValue(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, dtl_dv_t **ppInitValue);
DYN_STATIC const uint8_t* apx_attributeParser_parseArrayLength(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, uint32_t *pValue);
#endif


#endif //APX_ATTRIBUTE_PARSER_H
