/*****************************************************************************
* \file      node_manager.h
* \author    Conny Gustafsson
* \date      2019-12-29
* \brief     Manager for apx_node_instance objects
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
struct apx_connection_base_tag;
struct apx_node_instance_tag;

typedef struct apx_node_manager_tag
{
   apx_parser_t parser;
   apx_compiler_t compiler;
   apx_istream_t stream;
   adt_hash_t instance_map; //strong references to apx_node_instance objects. Key is the node name, value is of type apx_node_instance_t*
   apx_node_instance_t *last_attached; //weak reference
   apx_mode_t mode;
   struct apx_connection_base_tag* parent_connection; //Weak reference
   MUTEX_T lock; //locking mechanism
} apx_node_manager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_node_manager_create(apx_node_manager_t *self, apx_mode_t mode);
void apx_node_manager_destroy(apx_node_manager_t *self);
apx_node_manager_t *apx_node_manager_new(apx_mode_t mode);
void apx_node_manager_delete(apx_node_manager_t *self);

//client-side API (ALso used for unit tests)
apx_error_t apx_node_manager_build_node(apx_node_manager_t* self, char const* definition_text);

//server-side API
apx_error_t apx_node_manager_init_node_from_file_info(apx_node_manager_t* self, rmf_file_info_t const* file_info, bool* file_open_request);
apx_error_t apx_node_manager_build_node_from_data(apx_node_manager_t* self, apx_node_instance_t* node_instance);

//common API
struct apx_node_instance_tag* apx_node_manager_get_last_attached(apx_node_manager_t const* self);
apx_size_t apx_node_manager_length(apx_node_manager_t const* self);
adt_ary_t* apx_node_manager_get_nodes(apx_node_manager_t* self);
struct apx_node_instance_tag* apx_node_manager_find(apx_node_manager_t const* self, char const* name);
void apx_node_manager_set_connection(apx_node_manager_t* self, struct apx_connection_base_tag* connection);
struct apx_connection_base_tag* apx_node_manager_get_connection(apx_node_manager_t const* self);
apx_error_t apx_node_manager_on_definition_data_written(apx_node_manager_t* self, struct apx_node_instance_tag* node_instance, uint32_t offset, apx_size_t size);
void apx_node_manager_on_require_port_written(apx_node_manager_t* self, apx_port_instance_t *port_instance, uint8_t const* raw_data, apx_size_t data_size);
int32_t apx_node_manager_values(apx_node_manager_t* self, adt_ary_t* array);
int32_t apx_node_manager_get_error_line(apx_node_manager_t* self);

#endif //APX_NODE_INSTANCE_MANAGER_H
