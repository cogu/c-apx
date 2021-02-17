/*****************************************************************************
* \file      server.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX server class
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
#ifndef APX_SERVER_H
#define APX_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "apx/port_signature_map.h"
#include "apx/server_extension.h"
#include "apx/event_listener.h"
#include "apx/connection_manager.h"
#include "apx/event_loop.h"
#include "apx/node_instance.h"
#include "apx/port_connector_change_table.h"
#include "soa.h"
#include "adt_str.h"
#include "adt_ary.h"
#include "osmacro.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


typedef struct apx_server_tag
{
   adt_list_t server_event_listeners;          //Strong references to apx_serverEventListener_t
   apx_portSignatureMap_t port_signature_map;  //This is the global map that is used to build all port connectors.
                                               //Any access to this structure must be protected by acquiring the globalLock.
   apx_connectionManager_t connection_manager; //server connections
   adt_list_t extension_manager;               //TODO: replace with extensionManager class
   adt_ary_t modified_nodes;                   //Weak references to apx_nodeInstance_t. Used to keep track of which nodes have modified port connectors.
   THREAD_T event_thread;                      //Local worker thread (for playing server-global events such as log events)
   bool is_event_thread_valid;                 //True if event_thread is a valid variable
   soa_t allocator;                            //small object allocator
   apx_eventLoop_t event_loop;                  //Event loop used by event_thread
   MUTEX_T event_loop_lock;                    //For protecting the event loop
   MUTEX_T global_lock;                        //1. Protects the port_signature_map and connection_manager
                                               //2. Synchronize data routing
                                               //3. Controlling access to the global port_signature_map.
   MUTEX_T event_listener_lock;
#ifdef _WIN32
   unsigned int thread_id;
#endif
} apx_server_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self);
void apx_server_destroy(apx_server_t *self);
apx_server_t *apx_server_new(void);
void apx_server_delete(apx_server_t *self);
void apx_server_start(apx_server_t *self);
void apx_server_stop(apx_server_t *self);
void* apx_server_register_event_listener(apx_server_t *self, apx_serverEventListener_t *event_listener);
void apx_server_unregister_event_listener(apx_server_t *self, void *handle);

void apx_server_accept_connection(apx_server_t *self, apx_serverConnection_t *server_connection);
apx_error_t apx_server_detach_connection(apx_server_t *self, apx_serverConnection_t *server_connection);
apx_error_t apx_server_add_extension(apx_server_t *self, const char *name, apx_serverExtensionHandler_t *handler, dtl_dv_t *config);
void apx_server_log_write(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg);
apx_error_t apx_server_append_event(apx_server_t* self, apx_event_t* event);
void apx_server_take_global_lock(apx_server_t *self);
void apx_server_release_global_lock(apx_server_t *self);
apx_error_t apx_server_connect_node_instance_provide_ports(apx_server_t *self, apx_nodeInstance_t *node_instance);
apx_error_t apx_server_connect_node_instance_require_ports(apx_server_t *self, apx_nodeInstance_t *node_instance);
apx_error_t apx_server_disconnect_node_instance_provide_ports(apx_server_t *self, apx_nodeInstance_t *node_instance);
apx_error_t apx_server_disconnect_node_instance_require_ports(apx_server_t *self, apx_nodeInstance_t *node_instance);
apx_error_t apx_server_process_require_port_connector_changes(apx_server_t *self, apx_nodeInstance_t *require_node_instance, apx_portConnectorChangeTable_t *connector_changes);
apx_error_t apx_server_process_provide_port_connector_changes(apx_server_t *self, apx_nodeInstance_t *provide_node_instance, apx_portConnectorChangeTable_t *connector_changes);
apx_error_t apx_server_insert_modified_node_instance(apx_server_t *self, apx_nodeInstance_t *node_instance);
adt_ary_t *apx_server_get_modified_node_instance(const apx_server_t *self);
void apx_server_clear_port_connector_changes(apx_server_t *self);


#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self);
apx_serverConnection_t *apx_server_get_last_connection(apx_server_t const*self);
apx_portSignatureMap_t *apx_server_get_port_signature_map(apx_server_t const*self);
#endif


#endif //APX_SERVER_H
