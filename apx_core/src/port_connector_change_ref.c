/*****************************************************************************
* \file      port_connector_change_ref.c
* \author    Conny Gustafsson
* \date      2020-03-03
* \brief     Simple struct containing two pointers
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include "apx/port_connector_change_ref.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_port_connector_change_ref_create(apx_port_connector_change_ref_t *self, apx_node_instance_t * node_instance, apx_port_connector_change_table_t * connector_changes)
{
   if (self != NULL)
   {
      self->is_connector_changes_weak_ref = false; //By default, this data structure takes ownership of the connectorChanges variable
      self->node_instance = node_instance;
      self->connector_changes = connector_changes;
   }
}

void apx_port_connector_change_ref_destroy(apx_port_connector_change_ref_t *self)
{
   if (self != NULL)
   {
      if ( (!self->is_connector_changes_weak_ref) && (self->connector_changes != NULL) )
      {
         apx_port_connector_change_table_delete(self->connector_changes);
      }
   }
}

apx_port_connector_change_ref_t *apx_port_connector_change_ref_new(apx_node_instance_t *node_instance, apx_port_connector_change_table_t *connector_changes)
{
   apx_port_connector_change_ref_t *self = (apx_port_connector_change_ref_t*) malloc(sizeof(apx_port_connector_change_ref_t));
   if (self != NULL)
   {
      apx_port_connector_change_ref_create(self, node_instance, connector_changes);
   }
   return self;
}

void apx_port_connector_change_ref_delete(apx_port_connector_change_ref_t *self)
{
   if (self != NULL)
   {
      apx_port_connector_change_ref_destroy(self);
      free(self);
   }
}

void apx_port_connector_change_ref_vdelete(void *arg)
{
   apx_port_connector_change_ref_delete( (apx_port_connector_change_ref_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
