/*****************************************************************************
* \file      port_instance.h
* \author    Conny Gustafsson
* \date      2020-12-14
* \brief     Static information about an instantiated port (things that do not change during run-time)
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PORT_INSTANCE_H
#define APX_PORT_INSTANCE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/program.h"
#include "apx/computation.h"
#include "apx/data_element.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_node_instance_tag;

typedef struct apx_port_instance_tag
{
   //Members that requires serialization
   apx_program_t const* pack_program; //strong reference
   apx_program_t const* unpack_program; //strong reference
   char *name; //strong reference
   apx_data_element_t* effective_data_element; //Weak reference
   //Members that does not require serialization
   struct apx_node_instance_tag* parent;
   apx_port_type_t port_type;
   apx_port_id_t port_id;
   uint32_t data_offset;
   uint32_t data_size;
   uint32_t queue_length;
   uint32_t element_size; //Only used when m_queue_length > 0
   bool has_dynamic_data; //True if data_element has dynamic arrays anywhere in its definition
   apx_computation_list_t const* computation_list; //Weak reference (ownership is managed by parent node_instance)
   char* port_signature; //Only used in APX_SERVER_MODE
} apx_port_instance_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_port_instance_create(apx_port_instance_t* self, struct apx_node_instance_tag* parent, apx_port_type_t port_type, apx_port_id_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program);
void apx_port_instance_destroy(apx_port_instance_t* self);
apx_port_instance_t* apx_port_instance_new(struct apx_node_instance_tag* parent, apx_port_type_t port_type, apx_port_id_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program);
void apx_port_instance_delete(apx_port_instance_t* self);
void apx_port_instance_vdelete(void *arg);
struct apx_node_instance_tag* apx_port_instance_parent(apx_port_instance_t* self);
apx_port_type_t apx_port_instance_port_type(apx_port_instance_t* self);
apx_port_id_t apx_port_instance_port_id(apx_port_instance_t* self);
char const* apx_port_instance_name(apx_port_instance_t* self);
uint32_t apx_port_instance_data_offset(apx_port_instance_t const* self);
uint32_t apx_port_instance_data_size(apx_port_instance_t const* self);
uint32_t apx_port_instance_queue_length(apx_port_instance_t const* self);
uint32_t apx_port_instance_element_size(apx_port_instance_t const* self);
bool apx_port_instance_has_dynamic_data(apx_port_instance_t const* self);
apx_program_t const* apx_port_instance_pack_program(apx_port_instance_t* self);
apx_program_t const* apx_port_instance_unpack_program(apx_port_instance_t* self);
void apx_port_instance_set_effective_element(apx_port_instance_t* self, apx_data_element_t* data_element);
apx_data_element_t* apx_port_instance_get_effective_element(apx_port_instance_t* self);
apx_element_id_t apx_port_instance_element_id(apx_port_instance_t* self);
apx_error_t apx_port_instance_derive_properties(apx_port_instance_t* self, uint32_t offset, uint32_t *size);
void apx_port_instance_set_computation_list(apx_port_instance_t* self, apx_computation_list_t const* computation_list);
apx_computation_list_t const* apx_port_instance_get_computation_list(apx_port_instance_t* self);
apx_computation_t const* apx_port_instance_get_computation(apx_port_instance_t* self, int32_t index);
int32_t apx_port_instance_get_computation_list_length(apx_port_instance_t* self);
apx_computation_list_id_t apx_port_instance_get_computation_list_id(apx_port_instance_t* self);
apx_error_t apx_port_instance_create_port_signature(apx_port_instance_t* self);
char const* apx_port_instance_get_port_signature(apx_port_instance_t const* self, bool *has_dynamic_data);

#endif //APX_GUARD_H
