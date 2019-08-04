/*****************************************************************************
* \file      apx_nodeInfo.h
* \author    Conny Gustafsson
* \date      2019-01-04
* \brief     Description
*
* Copyright (c) 2019 Conny Gustafsson
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
#ifndef APX_NODE_INFO_H
#define APX_NODE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_node.h"
#include "apx_error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_parser_tag;


typedef struct apx_nodeInfo_tag
{
   apx_node_t *node; //strong reference
   char *text; //strong reference
   uint32_t textLen;
}apx_nodeInfo_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeInfo_create(apx_nodeInfo_t *self);
void apx_nodeInfo_destroy(apx_nodeInfo_t *self);
apx_nodeInfo_t* apx_nodeInfo_new(void);
void apx_nodeInfo_delete(apx_nodeInfo_t *self);
void apx_nodeInfo_vdelete(void *arg);

apx_error_t apx_nodeInfo_updateFromString(apx_nodeInfo_t *self, struct apx_parser_tag *parser, const char *apx_text);

#endif //APX_NODE_INFO_H
