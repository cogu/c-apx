/*****************************************************************************
* \file      apx_client.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2021 Conny Gustafsson
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
struct apx_fileManager_tag;
struct apx_nodeManager_tag;
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
   apx_clientConnection_t *connection; //message connection
   struct adt_list_tag *event_listeners; //weak references to apx_clientEventListener_t
   struct apx_nodeManager_tag *node_manager; //strong reference
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
apx_error_t apx_client_connect_testsocket(apx_client_t *self, struct testsocket_tag *socketObject);
#else
apx_error_t apx_client_connect_tcp(apx_client_t *self, const char *address, uint16_t port);
# ifndef _WIN32
apx_error_t apx_client_connect_unix(apx_client_t *self, const char *socketPath);
# endif
#endif
void apx_client_disconnect(apx_client_t *self);

void* apx_client_register_event_listener(apx_client_t *self, struct apx_clientEventListener_tag *listener);
void apx_client_unregister_event_listener(apx_client_t *self, void *handle);

int32_t apx_client_get_num_attached_nodes(apx_client_t *self);
int32_t apx_client_get_num_event_listeners(apx_client_t *self);
void apx_client_attach_connection(apx_client_t *self, apx_clientConnection_t *connection);
apx_clientConnection_t *apx_client_get_connection(apx_client_t *self);

apx_error_t apx_client_build_node(apx_client_t *self, const char *definition_text);
int32_t apx_client_get_error_line(apx_client_t *self);
apx_nodeInstance_t *apx_client_get_last_attached_node(apx_client_t *self);
struct apx_fileManager_tag *apx_client_get_file_manager(apx_client_t *self);
struct apx_nodeManager_tag *apx_client_get_node_manager(apx_client_t *self);

/*** Port Handle API ***/
apx_portInstance_t* apx_client_get_port_instance_by_name(apx_client_t *self, const char *node_name, const char *port_name);
apx_portInstance_t* apx_client_get_provide_port_instance_by_id(apx_client_t *self, const char *node_name, apx_portId_t port_id);
apx_portInstance_t* apx_client_get_require_port_instance_by_id(apx_client_t *self, const char *node_name, apx_portId_t port_id);

/*** Port Data Write API ***/
apx_error_t apx_client_write_port_data(apx_client_t *self, apx_portInstance_t* port_instance, const dtl_dv_t *value);
//apx_error_t apx_client_writePortData_u8(apx_client_t *self, void *portHandle, uint8_t value);
//apx_error_t apx_client_writePortData_u16(apx_client_t *self, void *portHandle, uint16_t value);
//apx_error_t apx_client_writePortData_u32(apx_client_t *self, void *portHandle, uint32_t value);

/*** Port Data Read API ***/
apx_error_t apx_client_read_port_data(apx_client_t *self, apx_portInstance_t* port_instance, dtl_dv_t **dv);
//apx_error_t apx_client_readPortData_u8(apx_client_t *self, void *portHandle, uint8_t *value);
//apx_error_t apx_client_readPortData_u16(apx_client_t *self, void *portHandle, uint16_t *value);
//apx_error_t apx_client_readPortData_u32(apx_client_t *self, void *portHandle, uint32_t *value);

#ifdef UNIT_TEST
void apx_client_run(apx_client_t *self);
#endif

#endif //APX_CLIENT_H
