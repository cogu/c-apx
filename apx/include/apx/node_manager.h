/*****************************************************************************
* \file      node_manager.h
* \author    Conny Gustafsson
* \date      2019-12-29
* \brief     Manager for apx_nodeInstance objects
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#include "apx/node_instance.h"
#include "apx/parser.h"
#include "apx/compiler.h"
#include "apx/error.h"
#include "apx/file_info.h"
#include "adt_hash.h"


#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "osmacro.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_connectionBase_tag;
struct apx_nodeInstance_tag;

typedef struct apx_nodeManager_tag
{
   apx_parser_t parser;
   apx_compiler_t compiler;
   apx_istream_t stream;
   adt_hash_t instance_map; //strong references to apx_nodeInstance objects. Key is the node name, value is of type apx_nodeInstance_t*
   apx_nodeInstance_t *last_attached; //weak reference
   apx_mode_t mode;
   struct apx_connectionBase_tag* parent_connection; //Weak reference
   MUTEX_T lock; //locking mechanism
} apx_nodeManager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeManager_create(apx_nodeManager_t *self, apx_mode_t mode);
void apx_nodeManager_destroy(apx_nodeManager_t *self);
apx_nodeManager_t *apx_nodeManager_new(apx_mode_t mode);
void apx_nodeManager_delete(apx_nodeManager_t *self);

//client-side API (ALso used for unit tests)
apx_error_t apx_nodeManager_build_node(apx_nodeManager_t* self, char const* definition_text);

//server-side API
apx_error_t apx_nodeManager_init_node_from_file_info(apx_nodeManager_t* self, rmf_fileInfo_t const* file_info, bool* file_open_request);
apx_error_t apx_nodeManager_build_node_from_data(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance);

//common API
struct apx_nodeInstance_tag* apx_nodeManager_get_last_attached(apx_nodeManager_t const* self);
apx_size_t apx_nodeManager_length(apx_nodeManager_t const* self);
adt_ary_t* apx_nodeManager_get_nodes(apx_nodeManager_t* self);
struct apx_nodeInstance_tag* apx_nodeManager_find(apx_nodeManager_t const* self, char const* name);
void apx_nodeManager_set_connection(apx_nodeManager_t* self, struct apx_connectionBase_tag* connection);
struct apx_connectionBase_tag* apx_nodeManager_get_connection(apx_nodeManager_t const* self);
apx_error_t apx_nodeManager_on_definition_data_written(apx_nodeManager_t* self, struct apx_nodeInstance_tag* node_instance, uint32_t offset, apx_size_t size);
void apx_nodeManager_on_require_port_data_written(apx_nodeManager_t* self, struct apx_nodeInstance_tag* node_instance, uint32_t offset, apx_size_t size);
void apx_nodeManager_on_provide_port_data_written(apx_nodeManager_t* self, struct apx_nodeInstance_tag* node_instance, uint32_t offset, apx_size_t size);
int32_t apx_nodeManager_values(apx_nodeManager_t* self, adt_ary_t* array);

#endif //APX_NODE_INSTANCE_MANAGER_H
