/*****************************************************************************
 * \file      apx_diag_client_notification.c
 * \author    Conny Gustafsson
 * \date      2026-10-30
 * \brief     Diagnostic notification lifecycle and cloning
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx_diag_client_notification.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

void apx_diag_client_notification_create(apx_diag_client_notification_t *self)
{
  assert(self != NULL);
  memset(self, 0, sizeof(apx_diag_client_notification_t));
  self->is_weak_ref = true;
}

void apx_diag_client_notification_destroy(apx_diag_client_notification_t *self)
{
  if (self != NULL)
  {
    if (!self->is_weak_ref)
    {
      free((void *)self->value);
    }
    apx_diag_client_notification_create(self);
  }
}

apx_diag_client_notification_t *apx_diag_client_notification_new(void)
{
  apx_diag_client_notification_t *self =
    (apx_diag_client_notification_t *)malloc(sizeof(apx_diag_client_notification_t));
  if (self != NULL)
  {
    apx_diag_client_notification_create(self);
  }
  return self;
}

void apx_diag_client_notification_delete(apx_diag_client_notification_t *self)
{
  if (self != NULL)
  {
    apx_diag_client_notification_destroy(self);
    free(self);
  }
}

void apx_diag_client_notification_vdelete(void *arg)
{
  apx_diag_client_notification_delete((apx_diag_client_notification_t *)arg);
}

apx_diag_client_notification_t *apx_diag_client_notification_clone(apx_diag_client_notification_t const *other)
{
  assert(other != NULL);
  assert(other->value_size == 0u || other->value != NULL);
  apx_diag_client_notification_t *self = apx_diag_client_notification_new();
  if (self != NULL)
  {
    uint8_t *value = NULL;
    if (other->value_size > 0u)
    {
      value = (uint8_t *)malloc(other->value_size);
      if (value == NULL)
      {
        apx_diag_client_notification_delete(self);
        return NULL;
      }
      memcpy(value, other->value, other->value_size);
    }
    *self = *other;
    self->value = value;
    self->is_weak_ref = false;
  }
  return self;
}
