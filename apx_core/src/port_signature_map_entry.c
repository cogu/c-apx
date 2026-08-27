/*****************************************************************************
* \file      port_signature_entry.c
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     An element in an apx_portSignatureMap_t
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#include <malloc.h>
#include <assert.h>
#include "apx/error.h"
#include "apx/port_signature_map_entry.h"
#include "apx/node_instance.h"
#include "apx/port_connector_change_table.h"
#include "apx/node_data.h"
#include "apx/port.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMapEntry_create(apx_portSignatureMapEntry_t *self)
{
   if ( self != NULL )
   {
      self->preferred_provider = (apx_portInstance_t*) NULL;
      adt_list_create(&self->require_ports, (void (*)(void*)) NULL);
      adt_list_create(&self->provide_ports, (void (*)(void*)) NULL);
   }
}

void apx_portSignatureMapEntry_destroy(apx_portSignatureMapEntry_t *self)
{
   if (self != NULL)
   {
      adt_list_destroy(&self->require_ports);
      adt_list_destroy(&self->provide_ports);
   }
}

apx_portSignatureMapEntry_t *apx_portSignatureMapEntry_new(void)
{
   apx_portSignatureMapEntry_t *self = (apx_portSignatureMapEntry_t*) malloc(sizeof(apx_portSignatureMapEntry_t));
   if(self != NULL)
   {
      apx_portSignatureMapEntry_create(self);
   }
   return self;
}

void apx_portSignatureMapEntry_delete(apx_portSignatureMapEntry_t *self)
{
   if (self != NULL)
   {
      apx_portSignatureMapEntry_destroy(self);
      free(self);
   }
}

void apx_portSignatureMapEntry_vdelete(void *arg)
{
   apx_portSignatureMapEntry_delete((apx_portSignatureMapEntry_t*) arg);
}

void apx_portSignatureMapEntry_attach_require_port(apx_portSignatureMapEntry_t* self, apx_portInstance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_insert(&self->require_ports, port_instance);
   }
}

void apx_portSignatureMapEntry_attach_provide_port(apx_portSignatureMapEntry_t* self, apx_portInstance_t* port_instance, bool is_preferred)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_insert(&self->provide_ports, port_instance);
      if (is_preferred)
      {
         apx_portSignatureMapEntry_set_preferred_provider(self, port_instance);
      }
   }
}

void apx_portSignatureMapEntry_detach_require_port(apx_portSignatureMapEntry_t* self, apx_portInstance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_remove(&self->require_ports, port_instance);
   }
}

void apx_portSignatureMapEntry_detach_provide_port(apx_portSignatureMapEntry_t* self, apx_portInstance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_remove(&self->provide_ports, port_instance);
   }
}


bool apx_portSignatureMapEntry_is_empty(apx_portSignatureMapEntry_t *self)
{
   if (self != NULL)
   {
      return (bool) ( adt_list_is_empty(&self->provide_ports) &&  adt_list_is_empty(&self->require_ports) );
   }
   return false;
}

int32_t apx_portSignatureMapEntry_get_num_providers(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->provide_ports);
   }
   return -1;
}

int32_t apx_portSignatureMapEntry_get_num_requesters(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->require_ports);
   }
   return -1;
}

apx_portInstance_t* apx_portSignatureMapEntry_get_first_provider(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return (apx_portInstance_t*) adt_list_first(&self->provide_ports);
   }
   return (apx_portInstance_t*) NULL;
}

apx_portInstance_t* apx_portSignatureMapEntry_get_last_provider(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return (apx_portInstance_t*) adt_list_last(&self->provide_ports);
   }
   return (apx_portInstance_t*) NULL;
}

apx_portInstance_t* apx_portSignatureMapEntry_get_first_requester(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return (apx_portInstance_t*)adt_list_first(&self->require_ports);
   }
   return (apx_portInstance_t*)NULL;
}

apx_portInstance_t* apx_portSignatureMapEntry_get_last_requester(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return (apx_portInstance_t*)adt_list_last(&self->require_ports);
   }
   return (apx_portInstance_t*)NULL;
}


void apx_portSignatureMapEntry_set_preferred_provider(apx_portSignatureMapEntry_t* self, apx_portInstance_t* port_instance)
{
   if ( (self != NULL) && (port_instance != NULL) )
   {
      self->preferred_provider = port_instance;
   }
}

apx_portInstance_t* apx_portSignatureMapEntry_get_preferred_provider(apx_portSignatureMapEntry_t* self)
{
   if (self != NULL)
   {
      return self->preferred_provider;
   }
   return (apx_portInstance_t*) 0;
}

apx_error_t apx_portSignatureMapEntry_notify_require_ports_about_provide_port_change(apx_portSignatureMapEntry_t* self, apx_portInstance_t* provide_port, apx_portConnectorEvent_t event_type)
{
   if ( (self != NULL) && (provide_port != NULL) && ( (event_type == APX_PORT_CONNECTED_EVENT) || (event_type == APX_PORT_DISCONNECTED_EVENT) ) )
   {
      apx_error_t retval = APX_NO_ERROR;
      if (adt_list_length(&self->require_ports) > 0)
      {
         adt_list_elem_t *iter;
         apx_portConnectorChangeTable_t *provide_port_change_table;
         apx_portConnectorChangeEntry_t *provide_port_change_entry;
         apx_portConnectorChangeEntry_actionFunc *action_func;
         assert(apx_portInstance_parent(provide_port) != NULL);

         action_func = (event_type == APX_PORT_CONNECTED_EVENT)? apx_portConnectorChangeEntry_add_connection : apx_portConnectorChangeEntry_remove_connection;

         provide_port_change_table = apx_nodeInstance_get_provide_port_connector_changes(apx_portInstance_parent(provide_port), true);
         assert(provide_port_change_table != 0);
         provide_port_change_entry = apx_portConnectorChangeTable_get_entry(provide_port_change_table, apx_portInstance_port_id(provide_port));
         assert(provide_port_change_entry != 0);

         for(iter = adt_list_iter_first(&self->require_ports); iter != NULL; iter = adt_list_iter_next(iter))
         {
            apx_portConnectorChangeTable_t *require_port_change_table;
            apx_portConnectorChangeEntry_t *require_port_change_entry;
            apx_portInstance_t *require_port = (apx_portInstance_t*) iter->pItem;
            assert(require_port != 0);
            assert(apx_portInstance_parent(require_port) != NULL);
            require_port_change_table = apx_nodeInstance_get_require_port_connector_changes(apx_portInstance_parent(require_port), true);
            assert(require_port_change_table != 0);
            require_port_change_entry = apx_portConnectorChangeTable_get_entry(require_port_change_table, apx_portInstance_port_id(require_port));
            assert(require_port_change_entry != 0);
            retval = action_func(require_port_change_entry, provide_port);
            if (retval == APX_NO_ERROR)
            {
               retval = action_func(provide_port_change_entry, require_port);
            }
            if (retval != APX_NO_ERROR)
            {
               break;
            }
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portSignatureMapEntry_notify_provide_ports_about_require_port_change(apx_portSignatureMapEntry_t* self, apx_portInstance_t* require_port, apx_portConnectorEvent_t event_type)
{
   if ( (self != NULL) && (require_port != NULL) && ( (event_type == APX_PORT_CONNECTED_EVENT) || (event_type == APX_PORT_DISCONNECTED_EVENT) ) )
   {
      apx_error_t retval = APX_NO_ERROR;
      if (adt_list_length(&self->provide_ports) > 0)
      {
         adt_list_elem_t *iter;
         apx_portConnectorChangeTable_t *require_port_change_table;
         apx_portConnectorChangeEntry_t *require_port_change_entry;
         apx_portConnectorChangeEntry_actionFunc *action_func;
         assert(apx_portInstance_parent(require_port) != NULL);

         action_func = (event_type == APX_PORT_CONNECTED_EVENT)? apx_portConnectorChangeEntry_add_connection : apx_portConnectorChangeEntry_remove_connection;

         require_port_change_table = apx_nodeInstance_get_require_port_connector_changes(apx_portInstance_parent(require_port), true);
         assert(require_port_change_table != NULL);
         require_port_change_entry = apx_portConnectorChangeTable_get_entry(require_port_change_table, apx_portInstance_port_id(require_port));
         assert(require_port_change_entry != NULL);

         for(iter = adt_list_iter_first(&self->provide_ports); iter != NULL; iter = adt_list_iter_next(iter))
         {
            apx_portConnectorChangeTable_t *provide_port_change_table;
            apx_portConnectorChangeEntry_t *provide_port_change_entry;
            apx_portInstance_t *provide_port = (apx_portInstance_t*) iter->pItem;
            assert(provide_port != NULL);
            assert(apx_portInstance_parent(provide_port) != NULL);
            provide_port_change_table = apx_nodeInstance_get_provide_port_connector_changes(apx_portInstance_parent(provide_port), true);
            assert(provide_port_change_table != 0);
            provide_port_change_entry = apx_portConnectorChangeTable_get_entry(provide_port_change_table, apx_portInstance_port_id(provide_port));
            assert(provide_port_change_entry != 0);
            retval = action_func(require_port_change_entry, provide_port);
            if (retval == APX_NO_ERROR)
            {
               retval = action_func(provide_port_change_entry, require_port);
            }
            if (retval != APX_NO_ERROR)
            {
               break;
            }

         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

