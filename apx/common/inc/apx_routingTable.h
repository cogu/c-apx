/*****************************************************************************
* \file      apx_routingTable.h
* \author    Conny Gustafsson
* \date      2018-10-08
* \brief     Global map of all port data elements
*
* Copyright (c) 2018 Conny Gustafsson
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
#ifndef APX_ROUTING_TABLE_H
#define APX_ROUTING_TABLE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_hash.h"
#include "adt_list.h"
#include "apx_types.h"
#include "apx_error.h"
#include "apx_routingTableEntry.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//Forward declarations
struct apx_nodeData_tag;
typedef struct apx_routingTable_tag
{
   adt_hash_t internalMap; //strong references to apx_routingTableEntry_t
   adt_list_t attachedNodes; //weak references to nodeData_t
   MUTEX_T mutex; //modification lock
}apx_routingTable_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_routingTable_create(apx_routingTable_t *self);
void apx_routingTable_destroy(apx_routingTable_t *self);

apx_routingTableEntry_t *apx_routingTable_find(apx_routingTable_t *self, const char *portSignature);
int32_t apx_routingTable_length(apx_routingTable_t *self);
void apx_routingTable_attachNodeData(apx_routingTable_t *self, struct apx_nodeData_tag *nodeData);
void apx_routingTable_detachNodeData(apx_routingTable_t *self, struct apx_nodeData_tag *nodeData);
void apx_routingTable_copyInitData(apx_routingTable_t *self, struct apx_nodeData_tag *nodeData);

#endif //APX_ROUTING_TABLE_H
