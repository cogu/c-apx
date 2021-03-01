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
         //Set non-overidable methods of connection_interface
         connection_interface->get_connection_id = apx_connectionBase_vget_connection_id;
         connection_interface->get_remotefile_protocol_version_id = apx_connectionBase_vget_remotefile_protocol_version_id;
         connection_interface->get_connection_type = apx_connectionBase_vget_connection_type;
         memcpy(&self->connection_interface, connection_interface, sizeof(apx_connectionInterface_t));
      }
      else
      {
         memset(&self->connection_interface, 0, sizeof(apx_connectionInterface_t));
      }
      self->node_manager = NULL;
      self->event_handler_arg = NULL;
      self->total_bytes_received = 0u;
      self->total_bytes_sent = 0u;
      self->connection_id = APX_INVALID_CONNECTION_ID;
      self->num_header_size = UINT32_SIZE;
      self->mode = mode;
      self->connection_type = APX_CONNECTION_TYPE_DEFAULT;
      self->rmf_version_id = RMF_PROTOCOL_VERSION_ID_1_0;
      rc = apx_allocator_create(&self->allocator, APX_MAX_NUM_MESSAGES);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      rc = apx_fileManager_create(&self->file_manager, mode, &self->connection_interface, &self->allocator);
      if (rc != APX_NO_ERROR)
      {
         apx_allocator_destroy(&self->allocator);
         return rc;
      }
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

void apx_connectionBase_start(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
#if APX_DEBUG_ENABLE
      if (self->mode == APX_SERVER_MODE)
      {
         printf("[BASE-CONNECTION %u] Starting connection\n", self->connection_id);
      }
      else
      {
         printf("[BASE-CONNECTION] Starting connection\n");
      }
#endif
      apx_fileManager_start(&self->file_manager);
      if (self->vtable.start != NULL)
      {
         self->vtable.start((void*)self);
      }
   }
}

void apx_connectionBase_stop(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
#if APX_DEBUG_ENABLE
      printf("[BASE-CONNECTION] Stopping connection\n");
#endif
#ifndef UNIT_TEST
      apx_fileManager_stop(&self->file_manager);
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

uint16_t apx_connectionBase_get_num_pending_worker_commands(apx_connectionBase_t* self)
{
   if (self != NULL)
   {
      return apx_fileManager_get_num_pending_worker_commands(&self->file_manager);
   }
   return 0u;
}

void apx_connectionBase_set_connection_id(apx_connectionBase_t* self, uint32_t connection_id)
{
   if (self != NULL)
   {
      self->connection_id = connection_id;
      //apx_fileManager_set_connection_id(&self->file_manager, connection_id);
   }
}

uint32_t apx_connectionBase_get_connection_id(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return self->connection_id;
   }
   return APX_INVALID_CONNECTION_ID;
}

void apx_connectionBase_set_connection_type(apx_connectionBase_t* self, apx_connectionType_t connection_type)
{
   if (self != NULL)
   {
      self->connection_type = connection_type;
   }
}

apx_connectionType_t apx_connectionBase_get_connection_type(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return self->connection_type;
   }
   return APX_CONNECTION_TYPE_DEFAULT;
}

void apx_connectionBase_set_num_header_size(apx_connectionBase_t* self, apx_size_t size)
{
   if ( (self != NULL) && ( (size == UINT16_SIZE) || (size == UINT32_SIZE) ))
   {
      self->num_header_size = size;
   }
}

apx_size_t apx_connectionBase_get_num_header_size(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return self->num_header_size;
   }
   return 0u;
}

void apx_connectionBase_set_rmf_proto_id(apx_connectionBase_t* self, rmf_versionId_t version_id)
{
   if (self != NULL)
   {
      self->rmf_version_id = version_id;
   }
}

rmf_versionId_t apx_connectionBase_get_rmf_proto_id(apx_connectionBase_t const* self)
{
   if (self != NULL)
   {
      return self->rmf_version_id;
   }
   return RMF_PROTOCOL_VERSION_ID_NONE;
}


//Virtual function call-points

void apx_connectionBase_node_created_notification(apx_connectionBase_t const* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL))
   {
      if ((self->vtable.node_created_notification != NULL))
      {
         self->vtable.node_created_notification((void*)self, node_instance);
      }
   }
}

void apx_connectionBase_require_port_write_notification(apx_connectionBase_t const* self, apx_portInstance_t* port_instance, uint8_t const* raw_data, apx_size_t data_size)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      if ((self->vtable.require_port_write_notification != NULL))
      {
         self->vtable.require_port_write_notification((void*)self, port_instance, raw_data, data_size);
      }
   }
}


uint32_t apx_connectionBase_vget_connection_id(void* arg)
{
   apx_connectionBase_t const* self = (apx_connectionBase_t const*) arg;
   if (self != NULL)
   {
      return self->connection_id;
   }
   return APX_INVALID_CONNECTION_ID;
}

rmf_versionId_t apx_connectionBase_vget_remotefile_protocol_version_id(void* arg)
{
   apx_connectionBase_t const* self = (apx_connectionBase_t const*)arg;
   if (self != NULL)
   {
      return self->rmf_version_id;
   }
   return RMF_PROTOCOL_VERSION_ID_NONE;
}

apx_connectionType_t apx_connectionBase_vget_connection_type(void* arg)
{
   apx_connectionBase_t const* self = (apx_connectionBase_t const*)arg;
   if (self != NULL)
   {
      return self->connection_type;
   }
   return APX_CONNECTION_TYPE_DEFAULT;
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
