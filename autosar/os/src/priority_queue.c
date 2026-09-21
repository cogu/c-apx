/*****************************************************************************
* \file      priority_queue.c
* \author    Conny Gustafsson
* \date      2017-06-28
* \brief     Priority queue implementation
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include "priority_queue.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
priority_queue_t* priority_queue_new(void)
{
   priority_queue_t *self = (priority_queue_t*) malloc(sizeof(priority_queue_t));
   if (self != NULL)
   {
      priority_queue_create(self);
   }
   return self;
}

void priority_queue_delete(priority_queue_t *self)
{
   if (self != NULL)
   {
      priority_queue_destroy(self);
      free(self);
   }
}

void priority_queue_vdelete(void *arg)
{
   priority_queue_delete((priority_queue_t*) arg);
}
void priority_queue_create(priority_queue_t *self)
{
   if (self != NULL)
   {
      adt_ary_create(&self->elements, adt_heap_elem_vdelete);
      self->startIndex = 0;
   }
}

void priority_queue_destroy(priority_queue_t *self)
{
   if (self != NULL)
   {
      adt_ary_destroy(&self->elements);
   }
}

//accessors
void priority_queue_push(priority_queue_t *self, void *pItem, uint32_t u32Priority)
{
   if (self != NULL)
   {
      adt_heap_elem_t *pElem = adt_heap_elem_new(pItem, u32Priority);
      if (pElem != NULL)
      {
         adt_ary_push(&self->elements, pElem);
         adt_heap_sortUp(&self->elements, adt_ary_length(&self->elements)-1, ADT_MIN_HEAP);
      }
   }
}

adt_heap_elem_t* priority_queue_pop(priority_queue_t *self)
{
   if (self != NULL)
   {
      void **ppElem;
      ppElem = adt_ary_shift(&self->elements);
      if (ppElem != NULL)
      {
         return (adt_heap_elem_t*) *ppElem;
      }
   }
   return NULL;
}

adt_heap_elem_t* priority_queue_top(priority_queue_t *self)
{
   if (self != NULL)
   {
      void **ppElem;
      ppElem = adt_ary_get(&self->elements, 0);
      if (ppElem != NULL)
      {
         return (adt_heap_elem_t*) *ppElem;
      }
   }
   return NULL;
}

uint32_t priority_queue_topPriority(priority_queue_t *self)
{
   if (self != NULL)
   {
      void **ppElem;
      ppElem = adt_ary_get(&self->elements, 0);
      if (ppElem != NULL)
      {
         adt_heap_elem_t *pElem = (adt_heap_elem_t*) ppElem;
         return pElem->u32Value;
      }
   }
   return 0;
}

void *priority_queue_topItem(priority_queue_t *self)
{
   if (self != NULL)
   {
      void **ppElem;
      ppElem = adt_ary_get(&self->elements, 0);
      if (ppElem != NULL)
      {
         adt_heap_elem_t *pElem = (adt_heap_elem_t*) ppElem;
         return pElem->pItem;
      }
   }
   return NULL;
}


//special function
///TODO: this function needs to support overflow situation, right now it doesn't
void priority_queue_incrementTopPriority(priority_queue_t *self, uint32_t value)
{
   if (self != NULL)
   {
      adt_heap_elem_t *pElem = priority_queue_top(self);
      if (pElem != NULL)
      {
         pElem->u32Value += value; ///FIXME: handle overflow situation
         adt_heap_sortDown(&self->elements, 0, ADT_MIN_HEAP);
      }
   }
}



//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


