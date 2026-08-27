/*****************************************************************************
* \file      node.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX (parse tree) node
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
#ifndef APX_NODE_H
#define APX_NODE_H
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "adt_hash.h"
#include "apx/data_type.h"
#include "apx/port.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_node_tag {
   adt_ary_t data_types; //strong reference to apx_dataType_t
   adt_ary_t require_ports; //strong reference to apx_port_t
   adt_ary_t provide_ports; //strong reference to apx_port_t
   adt_hash_t type_map; //weak reference to apx_dataType_t
   adt_hash_t port_map; //weak reference to apx_port_t
   char* name;
   bool is_finalized;
   int32_t last_error_line;
} apx_node_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_node_create(apx_node_t* self, const char* name);
void apx_node_destroy(apx_node_t* self);
apx_node_t* apx_node_new(const char* name);
void apx_node_delete(apx_node_t* self);
void apx_node_vdelete(void* arg);
apx_error_t apx_node_append_data_type(apx_node_t* self, apx_dataType_t* data_type);
apx_error_t apx_node_append_port(apx_node_t* self, apx_port_t* port);
void apx_node_set_name(apx_node_t* self, const char* name);
const char* apx_node_get_name(const apx_node_t* self);
int32_t apx_node_num_data_types(const apx_node_t* self);
int32_t apx_node_num_require_ports(const apx_node_t* self);
int32_t apx_node_num_provide_ports(const apx_node_t* self);
apx_dataType_t* apx_node_get_data_type(const apx_node_t* self, apx_typeId_t type_id);
apx_port_t* apx_node_get_require_port(const apx_node_t* self, apx_portId_t port_id);
apx_port_t* apx_node_get_provide_port(const apx_node_t* self, apx_portId_t port_id);

apx_dataType_t* apx_node_get_last_data_type(const apx_node_t* self);
apx_port_t* apx_node_get_last_require_port(const apx_node_t* self);
apx_port_t* apx_node_get_last_provide_port(const apx_node_t* self);
apx_error_t apx_node_finalize(apx_node_t* self);
int32_t apx_node_get_last_error_line(const apx_node_t* self);




#endif //APX_NODE_H