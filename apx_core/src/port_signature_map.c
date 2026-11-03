/*****************************************************************************
 * \file      port_signature_map.c
 * \author    Conny Gustafsson
 * \date      2020-02-18
 * \brief     Port signature map
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/port_signature_map.h"
#include "apx/node_instance.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_port_signature_map_connect_require_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance);
static apx_error_t apx_port_signature_map_connect_provide_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance);
static apx_error_t apx_port_signature_map_insert(
  apx_port_signature_map_t *self, const char *port_signature, apx_port_instance_t *port_instance);
static apx_port_signature_map_entry_t *apx_port_signature_map_create_new_entry(
  apx_port_signature_map_t *self, const char *port_signature);
static apx_error_t apx_port_signature_map_disconnect_require_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance);
static apx_error_t apx_port_signature_map_disconnect_provide_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance);
static apx_error_t apx_port_signature_map_remove(
  apx_port_signature_map_t *self, const char *port_signature, apx_port_instance_t *port_instance);
static void apx_port_signature_map_delete_entry(apx_port_signature_map_t *self, const char *port_signature);
static apx_port_instance_t *select_next_preferred_provider(
  apx_port_signature_map_t const *self, apx_port_signature_map_entry_t *entry);
static apx_error_t record_provider_change(apx_port_signature_map_t *self, apx_port_instance_t *old_provider,
  apx_port_instance_t *new_provider, apx_provider_change_reason_t reason);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_provider_change_t *apx_provider_change_new(
  apx_port_instance_t *old_provider, apx_port_instance_t *new_provider, apx_provider_change_reason_t reason)
{
  apx_provider_change_t *self = (apx_provider_change_t *)malloc(sizeof(apx_provider_change_t));
  if (self != NULL)
  {
    self->old_provider = old_provider;
    self->new_provider = new_provider;
    self->reason = reason;
  }
  return self;
}

void apx_provider_change_delete(apx_provider_change_t *self)
{
  if (self != NULL)
  {
    free(self);
  }
}

void apx_provider_change_vdelete(void *arg) { apx_provider_change_delete((apx_provider_change_t *)arg); }

void apx_port_signature_map_create(apx_port_signature_map_t *self)
{
  if (self != NULL)
  {
    adt_hash_create(&self->internal_map, apx_port_signature_map_entry_vdelete);
    adt_ary_create(&self->provider_changes, NULL);
    self->policy = APX_MULTIPLE_PROVIDER_POLICY_FIRST;
  }
}

void apx_port_signature_map_destroy(apx_port_signature_map_t *self)
{
  if (self != NULL)
  {
    adt_hash_destroy(&self->internal_map);
    apx_port_signature_map_clear_provider_changes(self);
    adt_ary_destroy(&self->provider_changes);
  }
}

void apx_port_signature_map_set_policy(apx_port_signature_map_t *self, apx_multiple_provider_policy_t policy)
{
  if (self != NULL)
  {
    self->policy = policy;
  }
}

apx_multiple_provider_policy_t apx_port_signature_map_get_policy(apx_port_signature_map_t const *self)
{
  if (self != NULL)
  {
    return self->policy;
  }
  return APX_MULTIPLE_PROVIDER_POLICY_FIRST;
}

adt_ary_t *apx_port_signature_map_get_provider_changes(apx_port_signature_map_t *self)
{
  if (self != NULL)
  {
    return &self->provider_changes;
  }
  return NULL;
}

void apx_port_signature_map_clear_provider_changes(apx_port_signature_map_t *self)
{
  if (self != NULL)
  {
    int32_t const len = adt_ary_length(&self->provider_changes);
    for (int32_t i = 0; i < len; i++)
    {
      apx_provider_change_t *change = (apx_provider_change_t *)adt_ary_value(&self->provider_changes, i);
      apx_provider_change_delete(change);
    }
    adt_ary_clear(&self->provider_changes);
  }
}

apx_port_signature_map_entry_t *apx_port_signature_map_find(apx_port_signature_map_t *self, const char *port_signature)
{
  if ((self != NULL) && (port_signature != 0))
  {
    void **ppResult = adt_hash_get(&self->internal_map, port_signature);
    if (ppResult != 0)
    {
      return (apx_port_signature_map_entry_t *)*ppResult;
    }
  }
  return NULL;
}

int32_t apx_port_signature_map_length(apx_port_signature_map_t *self)
{
  if (self != NULL)
  {
    return adt_hash_length(&self->internal_map);
  }
  return -1;
}

apx_port_signature_map_t *apx_port_signature_map_new(void)
{
  apx_port_signature_map_t *self = (apx_port_signature_map_t *)malloc(sizeof(apx_port_signature_map_t));
  if (self != NULL)
  {
    apx_port_signature_map_create(self);
  }
  return self;
}

void apx_port_signature_map_delete(apx_port_signature_map_t *self)
{
  if (self != NULL)
  {
    apx_port_signature_map_destroy(self);
    free(self);
  }
}

apx_error_t apx_port_signature_map_connect_provide_ports(
  apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance)
{
  if ((self != NULL) && (node_instance != NULL))
  {
    return apx_port_signature_map_connect_provide_ports_internal(self, node_instance);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_port_signature_map_connect_require_ports(
  apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance)
{
  if ((self != NULL) && (node_instance != NULL))
  {
    return apx_port_signature_map_connect_require_ports_internal(self, node_instance);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_port_signature_map_disconnect_provide_ports(
  apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance)
{
  if ((self != NULL) && (node_instance != NULL))
  {
    return apx_port_signature_map_disconnect_provide_ports_internal(self, node_instance);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_port_signature_map_disconnect_require_ports(
  apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance)
{
  if ((self != NULL) && (node_instance != NULL))
  {
    return apx_port_signature_map_disconnect_require_ports_internal(self, node_instance);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_port_signature_map_connect_require_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance)
{
  apx_port_id_t port_id;
  apx_size_t const num_require_ports = apx_node_instance_get_num_require_ports(node_instance);
  for (port_id = 0; port_id < num_require_ports; port_id++)
  {
    apx_port_instance_t *port_instance = apx_node_instance_get_require_port(node_instance, port_id);
    if (port_instance != NULL)
    {
      apx_error_t result = APX_NO_ERROR;
      bool has_dynamic_data = false;
      const char *port_signature = apx_port_instance_get_port_signature(port_instance, &has_dynamic_data);
      result = apx_port_signature_map_insert(self, port_signature, port_instance);
      if (result != APX_NO_ERROR)
      {
        return result;
      }
    }
  }
  return APX_NO_ERROR;
}

static apx_error_t apx_port_signature_map_connect_provide_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance)
{
  apx_port_id_t port_id;
  apx_size_t const num_provide_ports = apx_node_instance_get_num_provide_ports(node_instance);
  for (port_id = 0; port_id < num_provide_ports; port_id++)
  {
    apx_port_instance_t *port_instance = apx_node_instance_get_provide_port(node_instance, port_id);
    if (port_instance != NULL)
    {
      apx_error_t result = APX_NO_ERROR;
      bool has_dynamic_data = false;
      const char *port_signature = apx_port_instance_get_port_signature(port_instance, &has_dynamic_data);
      result = apx_port_signature_map_insert(self, port_signature, port_instance);
      if (result != APX_NO_ERROR)
      {
        return result;
      }
    }
  }
  return APX_NO_ERROR;
}

static apx_error_t apx_port_signature_map_insert(
  apx_port_signature_map_t *self, const char *port_signature, apx_port_instance_t *port_instance)
{
  apx_port_signature_map_entry_t *entry = NULL;
  assert(self != NULL);
  assert(port_instance != NULL);
  assert(strlen(port_signature) > 0);
  assert(port_instance != NULL);
  entry = apx_port_signature_map_find(self, port_signature);
  if (entry == NULL)
  {
    entry = apx_port_signature_map_create_new_entry(self, port_signature);
    if (entry == NULL)
    {
      return APX_MEM_ERROR;
    }
  }
  assert(entry != NULL);
  if (apx_port_instance_port_type(port_instance) == APX_PROVIDE_PORT)
  {
    apx_port_instance_t *old_preferred = apx_port_signature_map_entry_get_preferred_provider(entry);
    if (old_preferred == NULL)
    {
      apx_port_signature_map_entry_attach_provide_port(entry, port_instance, true);
      return apx_port_signature_map_entry_notify_require_ports_about_provide_port_change(
        entry, port_instance, APX_PORT_CONNECTED_EVENT);
    }
    else if (self->policy == APX_MULTIPLE_PROVIDER_POLICY_LAST)
    {
      apx_error_t rc;
      apx_port_signature_map_entry_attach_provide_port(entry, port_instance, true);
      rc = record_provider_change(self, old_preferred, port_instance, APX_PROVIDER_CHANGE_TAKEOVER);
      if (rc != APX_NO_ERROR)
      {
        return rc;
      }
      rc = apx_port_signature_map_entry_notify_provide_port_about_all_require_ports(
        entry, old_preferred, APX_PORT_DISCONNECTED_EVENT);
      if (rc != APX_NO_ERROR)
      {
        return rc;
      }
      return apx_port_signature_map_entry_notify_require_ports_about_provide_port_change(
        entry, port_instance, APX_PORT_CONNECTED_EVENT);
    }
    else
    {
      apx_port_signature_map_entry_attach_provide_port(entry, port_instance, false);
    }
  }
  else
  {
    apx_port_signature_map_entry_attach_require_port(entry, port_instance);
    return apx_port_signature_map_entry_notify_provide_ports_about_require_port_change(
      entry, port_instance, APX_PORT_CONNECTED_EVENT);
  }
  return APX_NO_ERROR;
}

static apx_port_signature_map_entry_t *apx_port_signature_map_create_new_entry(
  apx_port_signature_map_t *self, const char *port_signature)
{
  apx_port_signature_map_entry_t *entry = apx_port_signature_map_entry_new();
  if (entry != NULL)
  {
    adt_hash_set(&self->internal_map, port_signature, entry);
  }
  return entry;
}

static apx_error_t apx_port_signature_map_disconnect_require_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance)
{
  apx_port_id_t port_id;
  apx_size_t const num_require_ports = apx_node_instance_get_num_require_ports(node_instance);
  for (port_id = 0; port_id < num_require_ports; port_id++)
  {
    apx_port_instance_t *port_instance = apx_node_instance_get_require_port(node_instance, port_id);
    if (port_instance != NULL)
    {
      apx_error_t result = APX_NO_ERROR;
      bool has_dynamic_data = false;
      const char *port_signature = apx_port_instance_get_port_signature(port_instance, &has_dynamic_data);
      result = apx_port_signature_map_remove(self, port_signature, port_instance);
      if (result != APX_NO_ERROR)
      {
        return result;
      }
    }
  }
  return APX_NO_ERROR;
}

static apx_error_t apx_port_signature_map_disconnect_provide_ports_internal(
  apx_port_signature_map_t *self, apx_node_instance_t *node_instance)
{
  apx_port_id_t port_id;
  apx_size_t const num_provide_ports = apx_node_instance_get_num_provide_ports(node_instance);
  for (port_id = 0; port_id < num_provide_ports; port_id++)
  {
    apx_port_instance_t *port_instance = apx_node_instance_get_provide_port(node_instance, port_id);
    if (port_instance != NULL)
    {
      apx_error_t result = APX_NO_ERROR;
      bool has_dynamic_data = false;
      const char *port_signature = apx_port_instance_get_port_signature(port_instance, &has_dynamic_data);
      result = apx_port_signature_map_remove(self, port_signature, port_instance);
      if (result != APX_NO_ERROR)
      {
        return result;
      }
    }
  }
  return APX_NO_ERROR;
}


static apx_error_t apx_port_signature_map_remove(
  apx_port_signature_map_t *self, const char *port_signature, apx_port_instance_t *port_instance)
{
  apx_error_t retval = APX_NO_ERROR;
  apx_port_signature_map_entry_t *entry;
  assert(self != NULL);
  assert(port_signature != NULL);
  assert(strlen(port_signature) > 0);
  assert(port_instance != NULL);
  entry = apx_port_signature_map_find(self, port_signature);
  if (entry == NULL)
  {
    retval = APX_NOT_FOUND_ERROR;
  }
  else
  {
    if (apx_port_instance_port_type(port_instance) == APX_PROVIDE_PORT)
    {
      bool const was_preferred = (apx_port_signature_map_entry_get_preferred_provider(entry) == port_instance);
      apx_port_signature_map_entry_detach_provide_port(entry, port_instance);
      if (was_preferred)
      {
        apx_port_instance_t *new_preferred = select_next_preferred_provider(self, entry);
        apx_port_signature_map_entry_set_preferred_provider(entry, new_preferred);
        if (new_preferred != NULL)
        {
          retval = record_provider_change(self, port_instance, new_preferred, APX_PROVIDER_CHANGE_DISCONNECT);
        }
        if (retval == APX_NO_ERROR)
        {
          retval = apx_port_signature_map_entry_notify_require_ports_about_provide_port_change(
            entry, port_instance, APX_PORT_DISCONNECTED_EVENT);
        }
        if ((retval == APX_NO_ERROR) && (new_preferred != NULL))
        {
          retval = apx_port_signature_map_entry_notify_provide_port_about_all_require_ports(
            entry, new_preferred, APX_PORT_CONNECTED_EVENT);
        }
      }
    }
    else
    {
      apx_port_signature_map_entry_detach_require_port(entry, port_instance);
      retval = apx_port_signature_map_entry_notify_provide_ports_about_require_port_change(
        entry, port_instance, APX_PORT_DISCONNECTED_EVENT);
    }
    if (apx_port_signature_map_entry_is_empty(entry))
    {
      apx_port_signature_map_delete_entry(self, port_signature);
    }
  }
  return retval;
}

static void apx_port_signature_map_delete_entry(apx_port_signature_map_t *self, const char *port_signature)
{
  if ((self != NULL) && (port_signature != 0))
  {
    apx_port_signature_map_entry_t *entry =
      (apx_port_signature_map_entry_t *)adt_hash_remove(&self->internal_map, port_signature);
    if (entry != NULL)
    {
      apx_port_signature_map_entry_delete(entry);
    }
  }
}

static apx_port_instance_t *select_next_preferred_provider(
  apx_port_signature_map_t const *self, apx_port_signature_map_entry_t *entry)
{
  if ((self == NULL) || (entry == NULL))
  {
    return NULL;
  }
  if (self->policy == APX_MULTIPLE_PROVIDER_POLICY_LAST)
  {
    adt_list_elem_t *iter = adt_list_iter_last(&entry->provide_ports);
    while (iter != NULL)
    {
      apx_port_instance_t *candidate = (apx_port_instance_t *)iter->pItem;
      if ((candidate != NULL) && !apx_node_instance_is_closing(apx_port_instance_parent(candidate)))
      {
        return candidate;
      }
      iter = adt_list_iter_prev(iter);
    }
  }
  else
  {
    adt_list_elem_t *iter = adt_list_iter_first(&entry->provide_ports);
    while (iter != NULL)
    {
      apx_port_instance_t *candidate = (apx_port_instance_t *)iter->pItem;
      if ((candidate != NULL) && !apx_node_instance_is_closing(apx_port_instance_parent(candidate)))
      {
        return candidate;
      }
      iter = adt_list_iter_next(iter);
    }
  }
  return NULL;
}

static apx_error_t record_provider_change(apx_port_signature_map_t *self, apx_port_instance_t *old_provider,
  apx_port_instance_t *new_provider, apx_provider_change_reason_t reason)
{
  apx_provider_change_t *change = apx_provider_change_new(old_provider, new_provider, reason);
  if (change == NULL)
  {
    return APX_MEM_ERROR;
  }
  if (adt_ary_push(&self->provider_changes, change) != ADT_NO_ERROR)
  {
    apx_provider_change_delete(change);
    return APX_MEM_ERROR;
  }
  return APX_NO_ERROR;
}
