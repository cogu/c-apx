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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <assert.h>
#include "apx/connection_base.h"
//#include "apx/node_data.h"
//#include "apx/port_connector_change_table.h"
#include "apx/util.h"
#ifdef _WIN32
#include <process.h>
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

/*static apx_error_t apx_connectionBase_startWorkerThread(apx_connectionBase_t *self);
static void apx_connectionBase_stopWorkerThread(apx_connectionBase_t *self);
static void apx_connectionBase_stopWorkerThread(apx_connectionBase_t *self);
static apx_error_t apx_connectionBase_initTransmitHandler(apx_connectionBase_t *self);
static void apx_connectionBase_attach_node_instance(apx_connectionBase_t* self, apx_nodeInstance_t* nodeInstance);

//Internal event emit API

static void apx_connectionBase_emitFileCreatedEvent(apx_connectionBase_t *self, const apx_fileInfo_t *fileInfo);

//static void apx_connectionBase_createNodeCompleteEvent(apx_event_t *event, apx_nodeData_t *nodeData);
//static void apx_connectionBase_handlePortConnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType);
//static void apx_connectionBase_handlePortDisconnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType);


#ifndef UNIT_TEST
static THREAD_PROTO(eventHandlerWorkThread,arg);
#endif
*/

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_connectionBaseVTable_create(apx_connectionBaseVTable_t* self, apx_void_ptr_func_t* destructor, apx_void_ptr_func_t* start, apx_void_ptr_func_t* close)
{
   if (self != NULL)
   {
      memset(self, 0, sizeof(apx_connectionBaseVTable_t));
      self->destructor = destructor;
      self->start = start;
      self->close = close;
   }
}

apx_error_t apx_connectionBase_create(apx_connectionBase_t *self, apx_mode_t mode, apx_connectionBaseVTable_t* base_connection_vtable, apx_connectionInterface_t* connection_interface)
{
   if (self != NULL)
   {
      apx_error_t rc;
      if (base_connection_vtable != NULL)
      {
         memcpy(&self->vtable, base_connection_vtable, sizeof(apx_connectionBaseVTable_t));
      }
      else
      {
         memset(&self->vtable, 0, sizeof(apx_connectionBaseVTable_t));
      }
      if (connection_interface != NULL)
      {
         memcpy(&self->connection_interface, connection_interface, sizeof(apx_connectionInterface_t));
      }
      else
      {
         memset(&self->connection_interface, 0, sizeof(apx_connectionInterface_t));
      }
      self->node_manager = NULL;
      self->event_handler = NULL;
      self->event_handler_arg = NULL;
      self->total_bytes_received = 0u;
      self->total_bytes_sent = 0u;
      self->connection_id = APX_INVALID_CONNECTION_ID;
      self->num_header_size = UINT32_SIZE;
      self->mode = mode;
      self->event_loop_thread_valid = false;
#ifdef _WIN32
      self->event_loop_thread = INVALID_HANDLE_VALUE;
      self->thread_id = 0u;
#else
      self->event_loop_thread = 0;
#endif
      rc = apx_allocator_create(&self->allocator, APX_MAX_NUM_MESSAGES);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      rc = apx_eventLoop_create(&self->event_loop);
      if (rc != APX_NO_ERROR)
      {
         apx_allocator_destroy(&self->allocator);
         return APX_MEM_ERROR;
      }
      rc = apx_fileManager_create(&self->file_manager, mode, &self->connection_interface, &self->allocator);
      if (rc != APX_NO_ERROR)
      {
         apx_allocator_destroy(&self->allocator);
         apx_eventLoop_destroy(&self->event_loop);
         return rc;
      }
      adt_list_create(&self->connection_event_listeners, apx_connectionEventListener_vdelete);
      MUTEX_INIT(self->event_listener_mutex);
      apx_allocator_start(&self->allocator);
      return rc;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connectionBase_destroy(apx_connectionBase_t *self)
{
   if (self != NULL)
   {
      apx_fileManager_destroy(&self->file_manager);
      apx_eventLoop_destroy(&self->event_loop);
      MUTEX_DESTROY(self->event_listener_mutex);
      adt_list_destroy(&self->connection_event_listeners);
      apx_allocator_stop(&self->allocator);
      apx_allocator_destroy(&self->allocator);
   }
}

void apx_connectionBase_delete(apx_connectionBase_t *self)
{
   if(self != NULL)
   {
      if (self->vtable.destructor != 0)
      {
         self->vtable.destructor((void*) self);
      }
      free(self);
   }
}

void apx_connectionBase_vdelete(void *arg)
{
   apx_connectionBase_delete((apx_connectionBase_t*) arg);
}

apx_fileManager_t* apx_connectionBase_get_file_manager(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return (apx_fileManager_t*) &self->file_manager;
   }
   return NULL;
}

void apx_connectionBase_set_event_handler(apx_connectionBase_t* self, apx_eventHandlerFunc_t* event_handler, void* event_handler_arg)
{
   if (self != NULL)
   {
      self->event_handler = event_handler;
      self->event_handler_arg = event_handler_arg;
   }
}

void apx_connectionBase_start(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
#ifndef UNIT_TEST
      apx_connectionBase_start_worker_thread(self);
      apx_fileManager_start(&self->fileManager);
      if (self->vtable.start != NULL)
      {
         self->vtable.start((void*)self);
      }
#endif
   }
}

void apx_connectionBase_stop(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
#ifndef UNIT_TEST
      apx_fileManager_stop(&self->file_manager);
      apx_connectionBase_stop_worker_thread(self);
#endif
   }
}

void apx_connectionBase_close(apx_connectionBase_t* self)
{
   if ((self != NULL) && (self->vtable.close != NULL))
   {
      self->vtable.close((void*)self);
   }
}

void apx_connectionBase_attach_node_manager(apx_connectionBase_t* self, apx_nodeManager_t* node_manager)
{
   if ( (self != NULL) && (node_manager != NULL) )
   {
      apx_nodeManager_set_connection(node_manager, self);
      self->node_manager = node_manager;
   }
}

apx_nodeManager_t* apx_connectionBase_get_node_manager(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return self->node_manager;
   }
   return NULL;
}

apx_connectionInterface_t const* apx_connectionBase_get_connection(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return &self->connection_interface;
   }
   return NULL;
}

apx_error_t apx_connectionBase_message_received(apx_connectionBase_t* self, const uint8_t* data, apx_size_t size)
{
   if (self != NULL)
   {
      return apx_fileManager_message_received(&self->file_manager, data, size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connectionBase_node_created_notification(apx_connectionBase_t const* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      if ((self->vtable.node_created_notification != NULL))
      {
         self->vtable.node_created_notification((void*)self, node_instance);
      }
   }
}

uint16_t apx_connectionBase_get_num_pending_events(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
      return apx_eventLoop_numPendingEvents(&self->event_loop);
   }
   return 0;

}

uint16_t apx_connectionBase_get_num_pending_worker_commands(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
      return apx_fileManager_get_num_pending_worker_commands(&self->file_manager);
   }
   return 0u;
}

/*** Internal Callback API ***/

//Callbacks triggered due to events happening locally
void apx_connectionBase_disconnect_notification(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
      apx_fileManager_disconnected(&self->file_manager);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
