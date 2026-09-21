/*****************************************************************************
* \file      node_cache.h
* \author    Conny Gustafsson
* \date      2021-01-20
* \brief     Handles locally cached nodes
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_FILE_CACHE_H
#define APX_FILE_CACHE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_nodeCache_tag
{
   apx_mode_t mode;
} apx_nodeCache_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeCache_create(apx_nodeCache_t* self, apx_mode_t mode);
void apx_nodeCache_destroy(apx_nodeCache_t* self);
apx_nodeCache_t* apx_nodeCache_new(apx_mode_t mode);
void apx_nodeCache_delete(apx_nodeCache_t* self);

#endif //APX_FILE_CACHE_H
