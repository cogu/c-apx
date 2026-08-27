/*****************************************************************************
* \file      port_instance.h
* \author    Conny Gustafsson
* \date      2020-12-14
* \brief     Static information about an instantiated port (things that do not change during run-time)
*
* Copyright (c) 2020 Conny Gustafsson
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
struct apx_nodeInstance_tag;

typedef struct apx_portInstance_tag
{
   //Members that requires serialization
   apx_program_t const* pack_program; //strong reference
   apx_program_t const* unpack_program; //strong reference
   char *name; //strong reference
   apx_dataElement_t* effective_data_element; //Weak reference
   //Members that does not require serialization
   struct apx_nodeInstance_tag* parent;
   apx_portType_t port_type;
   apx_portId_t port_id;
   uint32_t data_offset;
   uint32_t data_size;
   uint32_t queue_length;
   uint32_t element_size; //Only used when m_queue_length > 0
   bool has_dynamic_data; //True if data_element has dynamic arrays anywhere in its definition
   apx_computationList_t const* computation_list; //Weak reference (ownership is managed by parent node_instance)
   char* port_signature; //Only used in APX_SERVER_MODE
} apx_portInstance_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portInstance_create(apx_portInstance_t* self, struct apx_nodeInstance_tag* parent, apx_portType_t port_type, apx_portId_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program);
void apx_portInstance_destroy(apx_portInstance_t* self);
apx_portInstance_t* apx_portInstance_new(struct apx_nodeInstance_tag* parent, apx_portType_t port_type, apx_portId_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program);
void apx_portInstance_delete(apx_portInstance_t* self);
void apx_portInstance_vdelete(void *arg);
struct apx_nodeInstance_tag* apx_portInstance_parent(apx_portInstance_t* self);
apx_portType_t apx_portInstance_port_type(apx_portInstance_t* self);
apx_portId_t apx_portInstance_port_id(apx_portInstance_t* self);
char const* apx_portInstance_name(apx_portInstance_t* self);
uint32_t apx_portInstance_data_offset(apx_portInstance_t const* self);
uint32_t apx_portInstance_data_size(apx_portInstance_t const* self);
uint32_t apx_portInstance_queue_length(apx_portInstance_t const* self);
uint32_t apx_portInstance_element_size(apx_portInstance_t const* self);
bool apx_portInstance_has_dynamic_data(apx_portInstance_t const* self);
apx_program_t const* apx_portInstance_pack_program(apx_portInstance_t* self);
apx_program_t const* apx_portInstance_unpack_program(apx_portInstance_t* self);
void apx_portInstance_set_effective_element(apx_portInstance_t* self, apx_dataElement_t* data_element);
apx_dataElement_t* apx_portInstance_get_effective_element(apx_portInstance_t* self);
apx_elementId_t apx_portInstance_element_id(apx_portInstance_t* self);
apx_error_t apx_portInstance_derive_properties(apx_portInstance_t* self, uint32_t offset, uint32_t *size);
void apx_portInstance_set_computation_list(apx_portInstance_t* self, apx_computationList_t const* computation_list);
apx_computationList_t const* apx_portInstance_get_computation_list(apx_portInstance_t* self);
apx_computation_t const* apx_portInstance_get_computation(apx_portInstance_t* self, int32_t index);
int32_t apx_portInstance_get_computation_list_length(apx_portInstance_t* self);
apx_computationListId_t apx_portInstance_get_computation_list_id(apx_portInstance_t* self);
apx_error_t apx_port_instance_create_port_signature(apx_portInstance_t* self);
char const* apx_portInstance_get_port_signature(apx_portInstance_t const* self, bool *has_dynamic_data);

#endif //APX_GUARD_H
