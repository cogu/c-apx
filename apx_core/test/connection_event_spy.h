/*****************************************************************************
* \file      connection_event_spy.h
* \author    Conny Gustafsson
* \date      2019-05-27
* \brief     Connection event spy
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_FILEMANAGER_EVENT_LISTENER_SPY_H
#define APX_FILEMANAGER_EVENT_LISTENER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/event_listener.h"
#include "apx/file_info.h"
#include "apx/connection_base.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_connection_event_spy_tag
{
   int32_t headerAcceptedCount;
   int32_t fileCreateCount;
   apx_connection_base_t *lastConnection;
   apx_file_info_t *lastFileInfo;
}apx_connection_event_spy_t;
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connection_event_spy_create(apx_connection_event_spy_t *self);
void apx_connection_event_spy_destroy(apx_connection_event_spy_t *self);
void apx_connection_event_spy_register(apx_connection_event_spy_t *self, apx_connection_base_t *connection);

void apx_connection_event_spy_header_accepted(void *arg, apx_connection_base_t *connection);
void apx_connection_event_spy_file_create(void *arg, apx_connection_base_t *connection, const apx_file_info_t *file_info);



#endif //APX_FILEMANAGER_EVENT_LISTENER_SPY_H
