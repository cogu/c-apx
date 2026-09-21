/*****************************************************************************
* \file      monitor_extension.h
* \author    Conny Gustafsson
* \date      2021-02-18
* \brief     Monitor extension
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_MONITOR_EXTENSION_H
#define APX_MONITOR_EXTENSION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server_extension.h"
#include "adt_list.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_MONITOR_EXTENSION_CFG_KEY "monitor"

struct apx_server_tag;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_monitor_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config);

#endif //APX_MONITOR_EXTENSION_H
