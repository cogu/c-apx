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
#include "apx/node.h"
#include "adt_ary.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_parser_tag
{
   adt_ary_t nodeList;
   apx_node_t *currentNode;
   apx_error_t lastErrorType;
   int32_t lastErrorLine;
   int16_t majorVersion;
   int16_t minorVersion;
}apx_parser_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_parser_create(apx_parser_t *self);
void apx_parser_destroy(apx_parser_t *self);
apx_parser_t* apx_parser_new(void);
void apx_parser_delete(apx_parser_t *self);
int32_t apx_parser_getNumNodes(apx_parser_t *self);
apx_node_t *apx_parser_getNode(apx_parser_t *self, int32_t index);
apx_error_t apx_parser_getLastError(apx_parser_t *self);
int32_t apx_parser_getErrorLine(apx_parser_t *self);
void apx_parser_clearNodes(apx_parser_t *self);
#if defined(_WIN32) || defined(__GNUC__)
apx_node_t *apx_parser_parseFile(apx_parser_t *self, const char *filename);
#endif
apx_node_t *apx_parser_parseString(apx_parser_t *self, const char *data);
apx_node_t *apx_parser_parseBuffer(apx_parser_t *self, const uint8_t *buf, apx_size_t len);

//event handlers
void apx_parser_open(apx_parser_t *self);
void apx_parser_close(apx_parser_t *self);
bool apx_parser_header(apx_parser_t *self, int16_t majorVersion, int16_t minorVersion);
void apx_parser_node(apx_parser_t *self, const char *name, int32_t lineNumber); //N"<name>"
int32_t apx_parser_datatype(apx_parser_t *self, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
int32_t apx_parser_require(apx_parser_t *self, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
int32_t apx_parser_provide(apx_parser_t *self, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
void apx_parser_node_end(apx_parser_t *self);
void apx_parser_parse_error(apx_parser_t *self, apx_error_t errorType, int32_t errorLine);

//void event handlers
void apx_parser_vopen(void *arg);
void apx_parser_vclose(void *arg);
bool apx_parser_vheader(void *arg, int16_t majorVersion, int16_t minorVersion);
void apx_parser_vnode(void *arg, const char *name, int32_t lineNumber); //N"<name>"
int32_t apx_parser_vdatatype(void *arg, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
int32_t apx_parser_vrequire(void *arg, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
int32_t apx_parser_vprovide(void *arg, const char *name, const char *dsg, const char *attr, int32_t lineNumber);
void apx_parser_vnode_end(void *arg);
void apx_parser_vparse_error(void *arg, apx_error_t errorType, int32_t errorLine);

#endif //APX_PARSER_H
