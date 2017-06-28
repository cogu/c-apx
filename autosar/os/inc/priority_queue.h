#ifndef OS_PQ_H
#define OS_PQ_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_heap.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct priority_queue_tag {
   adt_ary_t elements; //strong references to adt_heap_elem_t
   int32_t startIndex;
}priority_queue_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//constructor/destructor
priority_queue_t* priority_queue_new(void);
void priority_queue_delete(priority_queue_t *self);
void priority_queue_vdelete(void *arg);
void priority_queue_create(priority_queue_t *self);
void priority_queue_destroy(priority_queue_t *self);

//accessors
void priority_queue_push(priority_queue_t *self, void *pItem, uint32_t u32Priority);
adt_heap_elem_t* priority_queue_pop(priority_queue_t *self);
adt_heap_elem_t* priority_queue_top(priority_queue_t *self);
uint32_t priority_queue_topPriority(priority_queue_t *self);
void *priority_queue_topItem(priority_queue_t *self);

//special functions
void priority_queue_incrementTopPriority(priority_queue_t *self, uint32_t value);


#endif //OS_PQ_H
