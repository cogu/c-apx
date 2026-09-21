/*****************************************************************************
* \file      node_data.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Container for dynamic data of an APX node
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_NODE_DATA_H
#define APX_NODE_DATA_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/remotefile.h"
#ifndef APX_EMBEDDED
#  ifndef _WIN32
     //Linux-based system
#    include <pthread.h>
#  else
     //Windows-based system
#    include <Windows.h>
#  endif
#  include "osmacro.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_node_instance_tag;
struct apx_port_instance_tag;

typedef struct apx_node_data_buffers_tag
{
   uint8_t* definition_data;
   uint8_t* provide_port_data;
   uint8_t* require_port_data;
   apx_size_t definition_data_size;
   apx_size_t provide_port_data_size;
   apx_size_t require_port_data_size;
   apx_size_t num_provide_ports;
   apx_size_t num_require_ports;
   apx_port_count_t* require_port_connection_count;
   apx_port_count_t* provide_port_connection_count;
   uint8_t checksum_data[RMF_SHA256_SIZE];
   rmf_digest_type_t checksum_type;
} apx_node_data_buffers_t;

typedef struct apx_node_data_tag
{
   uint8_t* definition_data;
   uint8_t* require_port_data;
   uint8_t* provide_port_data;
   apx_size_t definition_data_size;
   apx_size_t require_port_data_size;
   apx_size_t provide_port_data_size;
   apx_size_t num_require_ports;
   apx_size_t num_provide_ports;
   apx_port_count_t* require_port_connection_count; //array-length: num_require_ports
   apx_port_count_t* provide_port_connection_count; //array-length: num_provide_ports
   uint8_t checksum_data[RMF_SHA256_SIZE];
   rmf_digest_type_t checksum_type;
   struct apx_node_instance_tag* parent;
   MUTEX_T lock;
   bool is_weak_ref; //true if memory is managed outside this object
} apx_node_data_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

////////////////// Constructor/Destructor //////////////////
apx_error_t apx_node_data_create(apx_node_data_t *self, apx_node_data_buffers_t *buffers);
void apx_node_data_destroy(apx_node_data_t *self);
apx_node_data_t *apx_node_data_new(void);
void apx_node_data_delete(apx_node_data_t *self);
void apx_node_data_vdelete(void *arg);

////////////////// Data API //////////////////
apx_size_t apx_node_data_definition_data_size(apx_node_data_t const* self);
apx_size_t apx_node_data_provide_port_data_size(apx_node_data_t const* self);
apx_size_t apx_node_data_require_port_data_size(apx_node_data_t const* self);
apx_size_t apx_node_data_num_provide_ports(apx_node_data_t const* self);
apx_size_t apx_node_data_num_require_ports(apx_node_data_t const* self);
apx_error_t apx_node_data_create_definition_data(apx_node_data_t* self, uint8_t const* init_data, apx_size_t data_size);
apx_error_t apx_node_data_create_provide_port_data(apx_node_data_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size);
apx_error_t apx_node_data_create_require_port_data(apx_node_data_t* self, apx_size_t num_ports, uint8_t const* init_data, apx_size_t data_size);
apx_error_t apx_node_data_write_definition_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size);
apx_error_t apx_node_data_write_provide_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size);
apx_error_t apx_node_data_read_provide_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size);
apx_error_t apx_node_data_write_require_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t const* src, apx_size_t size);
apx_error_t apx_node_data_read_require_port_data(apx_node_data_t* self, apx_size_t offset, uint8_t* dest, apx_size_t size);
uint8_t const* apx_node_data_get_definition_data(apx_node_data_t const* self);
/*uint8_t const* apx_node_data_get_provide_port_data(apx_node_data_t const* self);
uint8_t const* apx_node_data_get_require_port_data(apx_node_data_t const* self);*/
uint8_t* apx_node_data_take_definition_data_snapshot(apx_node_data_t* self);
uint8_t* apx_node_data_take_provide_port_data_snapshot(apx_node_data_t* self);
uint8_t* apx_node_data_take_require_port_data_snapshot(apx_node_data_t* self);
void apx_node_data_set_checksum_data(apx_node_data_t* self, rmf_digest_type_t checksum_type, uint8_t const* checksum_data);
rmf_digest_type_t apx_node_data_get_checksum_type(apx_node_data_t const* self);
const uint8_t* apx_node_data_get_checksum_data(apx_node_data_t const* self);


/*
////////////////// Port Connection Count API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_node_data_create_require_port_connection_count_buffer(apx_node_data_t* self, apx_size_t numRequirePorts);
apx_error_t apx_node_data_create_provide_port_connection_count_buffer(apx_node_data_t* self, apx_size_t numProvidePorts);
#endif
apx_connection_count_t apx_node_data_get_require_port_connection_count(apx_node_data_t* self, apx_port_id_t portId);
apx_connection_count_t apx_node_data_get_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t portId);
void apx_node_data_inc_require_port_connection_count(apx_node_data_t* self, apx_port_id_t portId);
void apx_node_data_inc_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t portId);
void apx_node_data_dec_require_port_connection_count(apx_node_data_t* self, apx_port_id_t portId);
void apx_node_data_dec_provide_port_connection_count(apx_node_data_t* self, apx_port_id_t portId);
uint32_t apx_node_data_get_port_connections_total(apx_node_data_t* self);
*/
/*
////////////////// Data Buffer API //////////////////
#ifndef APX_EMBEDDED
apx_error_t apx_node_data_create_definition_buffer(apx_node_data_t *self, apx_size_t bufferLen);
#endif
void apx_node_data_lock_definition_data(apx_node_data_t *self);
void apx_node_data_unlock_definition_data(apx_node_data_t *self);
const uint8_t *apx_node_data_get_definition_data_buf(apx_node_data_t *self);
apx_size_t apx_node_data_get_definition_data_len(apx_node_data_t *self);
apx_error_t apx_node_data_write_definition_data(apx_node_data_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_node_data_read_definition_data(apx_node_data_t *self, uint8_t *dest, uint32_t offset, uint32_t len);


#ifndef APX_EMBEDDED
apx_error_t apx_node_data_create_require_port_buffer(apx_node_data_t *self, apx_size_t bufferLen);
#endif
apx_size_t apx_node_data_get_require_port_data_len(apx_node_data_t *self);
apx_error_t apx_node_data_write_require_port_data(apx_node_data_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);
apx_error_t apx_node_data_read_require_port_data(apx_node_data_t *self, uint8_t *dest, uint32_t offset, apx_size_t len);


#ifndef APX_EMBEDDED
apx_error_t apx_node_data_create_provide_port_buffer(apx_node_data_t *self, apx_size_t bufferLen);
#endif
apx_size_t apx_node_data_get_provide_port_data_len(apx_node_data_t *self);
apx_error_t apx_node_data_write_provide_port_data(apx_node_data_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);
apx_error_t apx_node_data_read_provide_port_data(apx_node_data_t *self, uint8_t *dest, uint32_t offset, apx_size_t len);

apx_error_t apx_node_data_update_port_data_direct(apx_node_data_t *destNodeData, const struct apx_port_data_props_tag *destDatProps,
      apx_node_data_t *srcNodeData, const struct apx_port_data_props_tag *srcDataProps);

////////////////// NodeInstance (parent) API //////////////////
void apx_node_data_set_node_instance(apx_node_data_t *self, struct apx_node_instance_tag *node);
struct apx_node_instance_tag *apx_node_data_get_node_instance(apx_node_data_t *self);


////////////////// Utility Functions //////////////////
const char *apx_node_data_get_name(apx_node_data_t *self);
bool apx_node_data_is_complete(apx_node_data_t *self);
uint32_t apx_node_data_get_connection_id(apx_node_data_t *self);
*/

#endif //APX_NODE_DATA_H
