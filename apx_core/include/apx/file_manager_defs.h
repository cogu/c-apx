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
struct apx_node_data_tag;
struct apx_node_manager_tag;
struct apx_server_event_recorder_tag;
struct apx_server_event_player_tag;
struct apx_client_event_recorder_tag;
struct apx_client_event_player_tag;
struct apx_file_tag;

typedef struct apx_server_event_container_tag
{
   struct apx_server_event_recorder_tag *recorder;
   struct apx_server_event_player_tag *player;
}apx_server_event_container_t;

typedef struct apx_client_event_container_tag
{
   struct apx_client_event_recorder_tag *recorder;
   struct apx_client_event_player_tag *player;
}apx_client_event_container_t;


#endif //APX_FILE_MANAGER_COMMON_H
