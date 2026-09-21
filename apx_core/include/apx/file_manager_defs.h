/*****************************************************************************
* \file      file_manager_defs.h
* \author    Conny Gustafsson
* \date      2018-08-02
* \brief     APX FileManager common definitions
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_FILE_MANAGER_COMMON_H
#define APX_FILE_MANAGER_COMMON_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declarations
struct apx_nodeData_tag;
struct apx_nodeManager_tag;
struct apx_serverEventRecorder_tag;
struct apx_serverEventPlayer_tag;
struct apx_clientEventRecorder_tag;
struct apx_clientEventPlayer_tag;
struct apx_file_tag;

typedef struct apx_serverEventContainer_tag
{
   struct apx_serverEventRecorder_tag *recorder;
   struct apx_serverEventPlayer_tag *player;
}apx_serverEventContainer_t;

typedef struct apx_clientEventContainer_tag
{
   struct apx_clientEventRecorder_tag *recorder;
   struct apx_clientEventPlayer_tag *player;
}apx_clientEventContainer_t;


#endif //APX_FILE_MANAGER_COMMON_H
