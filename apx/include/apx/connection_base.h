/*****************************************************************************
* \file      apx_connection_base.h
* \author    Conny Gustafsson
* \date      2018-12-09
* \brief     Base class for all connections (client and server)
*
* Copyright (c) 2018-2021 Conny Gustafsson
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
#ifndef APX_CONNECTION_BASE_H
#define APX_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
# include <semaphore.h>
#endif
#include "apx/types.h"
#include "apx/error.h"
#include "apx/file_manager.h"
#include "apx/node_manager.h"
#include "apx/event_loop.h"
#include "apx/allocator.h"
#include "apx/connection_interface.h"
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeData_tag;
struct apx_portConnectionTable_tag;
struct apx_file_tag;
struct rmf_fileInfo_tag;


typedef void (apx_fileInfoNotifyFunc)(void *arg, const struct rmf_fileInfo_tag *fileInfo);
typedef void (apx_nodeFileWriteNotifyFunc)(void *arg, apx_nodeInstance_t * node_instance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);
typedef void (apx_nodeFileOpenNotifyFunc)(void *arg, apx_nodeInstance_t * node_instance, apx_fileType_t fileType);
typedef void (apx_portConnectorChangeCreateNotifyFunc)(void *arg, apx_nodeInstance_t * node_instance, apx_portType_t portType);
typedef void (apx_nodeCreatedFunc)(void* arg, apx_nodeInstance_t* node_instance);

typedef struct apx_connectionBaseVTable_tag
{
   apx_void_ptr_func_t* destructor;
   apx_void_ptr_func_t* start;
   apx_void_ptr_func_t* close;
   apx_nodeCreatedFunc* node_created_notification;
   apx_portConnectorChangeCreateNotifyFunc* port_connector_change_notify;
} apx_connectionBaseVTable_t;

typedef struct apx_connectionBase_tag
{
   apx_fileManager_t file_manager;
   apx_nodeManager_t* node_manager;
   apx_eventLoop_t event_loop;
   apx_allocator_t allocator;
   adt_list_t connection_event_listeners; //weak references to apx_connectionEventListener_t
   apx_connectionBaseVTable_t vtable;
   apx_connectionInterface_t connection_interface;
   MUTEX_T event_listener_mutex; //thread-protection for connection_event_listeners
   THREAD_T event_loop_thread;
   apx_eventHandlerFunc_t *event_handler;
   void *event_handler_arg;
   uint32_t total_bytes_received;
   uint32_t total_bytes_sent;
   uint32_t connection_id;
   apx_size_t num_header_size; //UINT16_SIZE or UINT32_SIZE
   apx_mode_t mode;
   bool event_loop_thread_valid;
#ifdef _WIN32
   unsigned int thread_id;
#endif
} apx_connectionBase_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connectionBaseVTable_create(apx_connectionBaseVTable_t *self, apx_void_ptr_func_t *destructor, apx_void_ptr_func_t *start, apx_void_ptr_func_t *close);
apx_error_t apx_connectionBase_create(apx_connectionBase_t *self, apx_mode_t mode, apx_connectionBaseVTable_t* base_connection_vtable, apx_connectionInterface_t* connection_interface);
void apx_connectionBase_destroy(apx_connectionBase_t *self);
void apx_connectionBase_delete(apx_connectionBase_t *self);
void apx_connectionBase_vdelete(void *arg);
apx_fileManager_t *apx_connectionBase_get_file_manager(apx_connectionBase_t const* self);
void apx_connectionBase_set_event_handler(apx_connectionBase_t* self, apx_eventHandlerFunc_t* event_handler, void* event_handler_arg);
void apx_connectionBase_start(apx_connectionBase_t *self);
void apx_connectionBase_stop(apx_connectionBase_t *self);
void apx_connectionBase_close(apx_connectionBase_t *self);
void apx_connectionBase_attach_node_manager(apx_connectionBase_t* self, apx_nodeManager_t* node_manager);
apx_nodeManager_t* apx_connectionBase_get_node_manager(apx_connectionBase_t const* self);
apx_connectionInterface_t const* apx_connectionBase_get_connection(apx_connectionBase_t const* self);
apx_error_t apx_connectionBase_message_received(apx_connectionBase_t *self, const uint8_t *data, apx_size_t size);
uint16_t apx_connectionBase_get_num_pending_events(apx_connectionBase_t *self);
uint16_t apx_connectionBase_get_num_pending_worker_commands(apx_connectionBase_t *self);

//uint8_t *apx_connectionBase_alloc(apx_connectionBase_t *self, size_t size);
//void apx_connectionBase_free(apx_connectionBase_t *self, uint8_t *ptr, size_t size);


//Virtual function call-points
void apx_connectionBase_node_created_notification(apx_connectionBase_t const* self, apx_nodeInstance_t* node_instance);

/*** Internal Callback API ***/

//Callbacks triggered due to events happening locally
void apx_connectionBase_disconnect_notification(apx_connectionBase_t *self);


#ifdef UNIT_TEST
void apx_connectionBase_run(apx_connectionBase_t *self);
#endif


#endif //APX_CONNECTION_BASE_H
