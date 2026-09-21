/*****************************************************************************
* \file      client.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CLIENT_H
#define APX_CLIENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx/error.h"
#include "apx/client_connection.h"
#include "apx/node_instance.h"
#include "apx/event_listener.h"
#include "apx/port_instance.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations

struct adt_ary_tag;
struct adt_list_tag;
struct adt_hash_tag;
struct apx_file_manager_tag;
struct apx_node_manager_tag;
struct apx_vm_tag;

#ifndef APX_EMBEDDED
# ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
# else
#  include <pthread.h>
# endif
#include "osmacro.h"
#endif

#ifdef UNIT_TEST
struct testsocket_tag;
#endif

typedef struct apx_client_tag
{
   apx_client_connection_t *connection; //message connection
   struct adt_list_tag *event_listeners; //weak references to apx_client_event_listener_t
   struct apx_node_manager_tag *node_manager; //strong reference
   struct apx_vm_tag *vm; //strong refence
   MUTEX_T lock;
   MUTEX_T event_listener_lock;
   bool is_connected;
} apx_client_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_client_create(apx_client_t *self);
void apx_client_destroy(apx_client_t *self);
apx_client_t *apx_client_new(void);
void apx_client_delete(apx_client_t *self);
void apx_client_vdelete(void *arg);

#ifdef UNIT_TEST
apx_error_t apx_client_connect_testsocket(apx_client_t *self, struct testsocket_tag *socket_object);
#else
apx_error_t apx_client_connect_tcp(apx_client_t *self, const char *address, uint16_t port);
# ifndef _WIN32
apx_error_t apx_client_connect_unix(apx_client_t *self, const char *socket_path);
# endif
#endif
void apx_client_disconnect(apx_client_t *self);

void* apx_client_register_event_listener(apx_client_t *self, struct apx_client_event_listener_tag *listener);
void apx_client_unregister_event_listener(apx_client_t *self, void *handle);

int32_t apx_client_get_num_attached_nodes(apx_client_t *self);
int32_t apx_client_get_num_event_listeners(apx_client_t *self);
void apx_client_attach_connection(apx_client_t *self, apx_client_connection_t *connection);
apx_client_connection_t *apx_client_get_connection(apx_client_t *self);

apx_error_t apx_client_build_node(apx_client_t *self, const char *definition_text);
int32_t apx_client_get_error_line(apx_client_t *self);
apx_node_instance_t *apx_client_get_last_attached_node(apx_client_t *self);
struct apx_file_manager_tag *apx_client_get_file_manager(apx_client_t *self);
struct apx_node_manager_tag *apx_client_get_node_manager(apx_client_t *self);

/*** Port Handle API ***/
apx_port_instance_t* apx_client_get_port_instance_by_name(apx_client_t *self, const char *node_name, const char *port_name);
apx_port_instance_t* apx_client_get_provide_port_instance_by_id(apx_client_t *self, const char *node_name, apx_port_id_t port_id);
apx_port_instance_t* apx_client_get_require_port_instance_by_id(apx_client_t *self, const char *node_name, apx_port_id_t port_id);

/*** Port Data Write API ***/
apx_error_t apx_client_write_port_data(apx_client_t *self, apx_port_instance_t* port_instance, const dtl_dv_t *value);
//apx_error_t apx_client_write_port_data_u8(apx_client_t *self, void *portHandle, uint8_t value);
//apx_error_t apx_client_write_port_data_u16(apx_client_t *self, void *portHandle, uint16_t value);
//apx_error_t apx_client_write_port_data_u32(apx_client_t *self, void *portHandle, uint32_t value);

/*** Port Data Read API ***/
apx_error_t apx_client_read_port_data(apx_client_t *self, apx_port_instance_t* port_instance, dtl_dv_t **dv);
//apx_error_t apx_client_read_port_data_u8(apx_client_t *self, void *portHandle, uint8_t *value);
//apx_error_t apx_client_read_port_data_u16(apx_client_t *self, void *portHandle, uint16_t *value);
//apx_error_t apx_client_read_port_data_u32(apx_client_t *self, void *portHandle, uint32_t *value);

#ifdef UNIT_TEST
void apx_client_run(apx_client_t *self);
#endif

#endif //APX_CLIENT_H
