/*****************************************************************************
* \file      apx_nodeManager.h
* \author    Conny Gustafsson
* \date      2019-12-29
* \brief     Manager for apx_nodeInstance objects
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
#ifndef APX_NODE_MANAGER_H
#define APX_NODE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_nodeInstance.h"
#include "apx_parser.h"
#include "apx_stream.h"
#include "apx_error.h"
#include "adt_hash.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "osmacro.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
//struct apx_node_tag;

typedef struct apx_nodeManager_tag
{
   apx_parser_t parser;
   apx_istream_t apx_istream; //helper structure for parser
   adt_hash_t nodeInstanceMap; //references to apx_nodeInstance objects. Key is the node name, value is of type apx_nodeInstance_t*
   SPINLOCK_T lock; //locking mechanism
   apx_nodeInstance_t *lastAttached; //weak reference
   apx_mode_t mode;
} apx_nodeManager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeManager_create(apx_nodeManager_t *self, apx_mode_t mode, bool useWeakRef); //defaultValue for useWeakRef=false
void apx_nodeManager_destroy(apx_nodeManager_t *self);
apx_nodeManager_t *apx_nodeManager_new(apx_mode_t mode, bool useWeakRef);
void apx_nodeManager_delete(apx_nodeManager_t *self);

/********** Client mode API  ************/
apx_error_t apx_nodeManager_buildNode_cstr(apx_nodeManager_t *self, const char *definition_text); //used when useWeakRef: false
apx_error_t apx_nodeManager_attachNode(apx_nodeManager_t *self, apx_nodeInstance_t *nodeInstance); //Used when useWeakRef: true

/********** Server mode API  ************/
apx_nodeInstance_t *apx_nodeManager_createNode(apx_nodeManager_t *self, const char *nodeName);

/********** Utility functions  ************/
apx_nodeInstance_t *apx_nodeManager_find(apx_nodeManager_t *self, const char *name);
int32_t apx_nodeManager_length(apx_nodeManager_t *self);
int32_t apx_nodeManager_keys(apx_nodeManager_t *self, adt_ary_t* array);
int32_t apx_nodeManager_values(apx_nodeManager_t *self, adt_ary_t* array);
apx_nodeInstance_t *apx_nodeManager_getLastAttached(apx_nodeManager_t *self);


/*
apx_error_t apx_nodeManager_getLastError(apx_nodeManager_t *self);
int32_t apx_nodeManager_getErrorLine(apx_nodeManager_t *self);

apx_error_t apx_nodeManager_parseDefinition(apx_nodeManager_t *self, apx_nodeData_t *nodeData);
apx_error_t apx_nodeManager_attach(apx_nodeManager_t *self, apx_nodeData_t *nodeData);
apx_error_t apx_nodeManager_attachFromString(apx_nodeManager_t *self, const char *apx_text);

apx_nodeData_t *apx_nodeManager_getLastAttached(apx_nodeManager_t *self);
int32_t apx_nodeManager_length(apx_nodeManager_t *self);
int32_t apx_nodeManager_keys(apx_nodeManager_t *self, adt_ary_t* array);
int32_t apx_nodeManager_values(apx_nodeManager_t *self, adt_ary_t* array);
*/

#endif //APX_NODE_INSTANCE_MANAGER_H
