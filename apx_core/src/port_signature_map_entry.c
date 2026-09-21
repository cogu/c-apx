/*****************************************************************************
* \file      port_signature_map_entry.c
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     An element in an apx_port_signature_map_t
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
void apx_port_signature_map_entry_create(apx_port_signature_map_entry_t *self)
{
   if ( self != NULL )
   {
      self->preferred_provider = (apx_port_instance_t*) NULL;
      adt_list_create(&self->require_ports, (void (*)(void*)) NULL);
      adt_list_create(&self->provide_ports, (void (*)(void*)) NULL);
   }
}

void apx_port_signature_map_entry_destroy(apx_port_signature_map_entry_t *self)
{
   if (self != NULL)
   {
      adt_list_destroy(&self->require_ports);
      adt_list_destroy(&self->provide_ports);
   }
}

apx_port_signature_map_entry_t *apx_port_signature_map_entry_new(void)
{
   apx_port_signature_map_entry_t *self = (apx_port_signature_map_entry_t*) malloc(sizeof(apx_port_signature_map_entry_t));
   if(self != NULL)
   {
      apx_port_signature_map_entry_create(self);
   }
   return self;
}

void apx_port_signature_map_entry_delete(apx_port_signature_map_entry_t *self)
{
   if (self != NULL)
   {
      apx_port_signature_map_entry_destroy(self);
      free(self);
   }
}

void apx_port_signature_map_entry_vdelete(void *arg)
{
   apx_port_signature_map_entry_delete((apx_port_signature_map_entry_t*) arg);
}

void apx_port_signature_map_entry_attach_require_port(apx_port_signature_map_entry_t* self, apx_port_instance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_insert(&self->require_ports, port_instance);
   }
}

void apx_port_signature_map_entry_attach_provide_port(apx_port_signature_map_entry_t* self, apx_port_instance_t* port_instance, bool is_preferred)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_insert(&self->provide_ports, port_instance);
      if (is_preferred)
      {
         apx_port_signature_map_entry_set_preferred_provider(self, port_instance);
      }
   }
}

void apx_port_signature_map_entry_detach_require_port(apx_port_signature_map_entry_t* self, apx_port_instance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_remove(&self->require_ports, port_instance);
   }
}

void apx_port_signature_map_entry_detach_provide_port(apx_port_signature_map_entry_t* self, apx_port_instance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      adt_list_remove(&self->provide_ports, port_instance);
   }
}


bool apx_port_signature_map_entry_is_empty(apx_port_signature_map_entry_t *self)
{
   if (self != NULL)
   {
      return (bool) ( adt_list_is_empty(&self->provide_ports) &&  adt_list_is_empty(&self->require_ports) );
   }
   return false;
}

int32_t apx_port_signature_map_entry_get_num_providers(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->provide_ports);
   }
   return -1;
}

int32_t apx_port_signature_map_entry_get_num_requesters(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->require_ports);
   }
   return -1;
}

apx_port_instance_t* apx_port_signature_map_entry_get_first_provider(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return (apx_port_instance_t*) adt_list_first(&self->provide_ports);
   }
   return (apx_port_instance_t*) NULL;
}

apx_port_instance_t* apx_port_signature_map_entry_get_last_provider(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return (apx_port_instance_t*) adt_list_last(&self->provide_ports);
   }
   return (apx_port_instance_t*) NULL;
}

apx_port_instance_t* apx_port_signature_map_entry_get_first_requester(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return (apx_port_instance_t*)adt_list_first(&self->require_ports);
   }
   return (apx_port_instance_t*)NULL;
}

apx_port_instance_t* apx_port_signature_map_entry_get_last_requester(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return (apx_port_instance_t*)adt_list_last(&self->require_ports);
   }
   return (apx_port_instance_t*)NULL;
}


void apx_port_signature_map_entry_set_preferred_provider(apx_port_signature_map_entry_t* self, apx_port_instance_t* port_instance)
{
   if ( (self != NULL) && (port_instance != NULL) )
   {
      self->preferred_provider = port_instance;
   }
}

apx_port_instance_t* apx_port_signature_map_entry_get_preferred_provider(apx_port_signature_map_entry_t* self)
{
   if (self != NULL)
   {
      return self->preferred_provider;
   }
   return NULL;
}

apx_error_t apx_port_signature_map_entry_notify_require_ports_about_provide_port_change(apx_port_signature_map_entry_t* self, apx_port_instance_t* provide_port, apx_port_connector_event_t event_type)
{
   if ( (self != NULL) && (provide_port != NULL) && ( (event_type == APX_PORT_CONNECTED_EVENT) || (event_type == APX_PORT_DISCONNECTED_EVENT) ) )
   {
      apx_error_t retval = APX_NO_ERROR;
      if (adt_list_length(&self->require_ports) > 0)
      {
         adt_list_elem_t *iter;
         apx_port_connector_change_table_t *provide_port_change_table;
         apx_port_connector_change_entry_t *provide_port_change_entry;
         apx_port_connector_change_entry_action_func_t *action_func;
         assert(apx_port_instance_parent(provide_port) != NULL);

         action_func = (event_type == APX_PORT_CONNECTED_EVENT)? apx_port_connector_change_entry_add_connection : apx_port_connector_change_entry_remove_connection;

         provide_port_change_table = apx_node_instance_get_provide_port_connector_changes(apx_port_instance_parent(provide_port), true);
         assert(provide_port_change_table != NULL);
         provide_port_change_entry = apx_port_connector_change_table_get_entry(provide_port_change_table, apx_port_instance_port_id(provide_port));
         assert(provide_port_change_entry != NULL);

         for(iter = adt_list_iter_first(&self->require_ports); iter != NULL; iter = adt_list_iter_next(iter))
         {
            apx_port_connector_change_table_t *require_port_change_table;
            apx_port_connector_change_entry_t *require_port_change_entry;
            apx_port_instance_t *require_port = (apx_port_instance_t*) iter->pItem;
            assert(require_port != NULL);
            assert(apx_port_instance_parent(require_port) != NULL);
            require_port_change_table = apx_node_instance_get_require_port_connector_changes(apx_port_instance_parent(require_port), true);
            assert(require_port_change_table != 0);
            require_port_change_entry = apx_port_connector_change_table_get_entry(require_port_change_table, apx_port_instance_port_id(require_port));
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

apx_error_t apx_port_signature_map_entry_notify_provide_ports_about_require_port_change(apx_port_signature_map_entry_t* self, apx_port_instance_t* require_port, apx_port_connector_event_t event_type)
{
   if ( (self != NULL) && (require_port != NULL) && ( (event_type == APX_PORT_CONNECTED_EVENT) || (event_type == APX_PORT_DISCONNECTED_EVENT) ) )
   {
      apx_error_t retval = APX_NO_ERROR;
      if (adt_list_length(&self->provide_ports) > 0)
      {
         adt_list_elem_t *iter;
         apx_port_connector_change_table_t *require_port_change_table;
         apx_port_connector_change_entry_t *require_port_change_entry;
         apx_port_connector_change_entry_action_func_t *action_func;
         assert(apx_port_instance_parent(require_port) != NULL);

         action_func = (event_type == APX_PORT_CONNECTED_EVENT)? apx_port_connector_change_entry_add_connection : apx_port_connector_change_entry_remove_connection;

         require_port_change_table = apx_node_instance_get_require_port_connector_changes(apx_port_instance_parent(require_port), true);
         assert(require_port_change_table != NULL);
         require_port_change_entry = apx_port_connector_change_table_get_entry(require_port_change_table, apx_port_instance_port_id(require_port));
         assert(require_port_change_entry != NULL);

         for(iter = adt_list_iter_first(&self->provide_ports); iter != NULL; iter = adt_list_iter_next(iter))
         {
            apx_port_connector_change_table_t *provide_port_change_table;
            apx_port_connector_change_entry_t *provide_port_change_entry;
            apx_port_instance_t *provide_port = (apx_port_instance_t*) iter->pItem;
            assert(provide_port != NULL);
            assert(apx_port_instance_parent(provide_port) != NULL);
            provide_port_change_table = apx_node_instance_get_provide_port_connector_changes(apx_port_instance_parent(provide_port), true);
            assert(provide_port_change_table != NULL);
            provide_port_change_entry = apx_port_connector_change_table_get_entry(provide_port_change_table, apx_port_instance_port_id(provide_port));
            assert(provide_port_change_entry != NULL);
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

